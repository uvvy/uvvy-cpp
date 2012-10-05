
#include <math.h>

#include <QStringList>
#include <QMutexLocker>
#include <QtDebug>

#include "audio.h"
#include "portaudio.h"


////////// Audio //////////

// -1 indicates we haven't called Pa_Initialize()
int Audio::ndevs = -1;

int Audio::indev = -1;
int Audio::outdev = -1;
QString Audio::indevname;
QString Audio::outdevname;

QList<AbstractAudioInput*> Audio::instreams;	// Enabled input streams
QList<AbstractAudioOutput*> Audio::outstreams;	// Enabled output streams

// Last error string returned
QString Audio::errmsg;

static PortAudioStream *pas;
static double parate;
static int paframesize;

static int inlev, outlev;

static AudioInputMonitor *inmon;


static const int monSampleRate = 8000;
static const int monFrameSize = 160;


Audio *Audio::instance()
{
	static Audio *obj;
	if (!obj)
		obj = new Audio();
	return obj;
}

Audio::Audio()
{
	inmon = new AudioInputMonitor();
	inmon->setSampleRate(monSampleRate);
	inmon->setFrameSize(monFrameSize);
}

int Audio::scan()
{
	if (ndevs >= 0)
		return ndevs;

	int rc = Pa_Initialize();
	if (rc) {
		errmsg = Pa_GetErrorText(rc);
		qWarning("Can't initialize PortAudio library: %s",
			Pa_GetErrorText(rc));
		return -1;
	}

	ndevs = Pa_CountDevices();
	if (ndevs == 0)
		errmsg = tr("No audio devices available");

	if (indevname.isEmpty() || (indev = findDevice(indevname)) < 0
			|| inChannels(indev) <= 0)
		indev = defaultInputDevice();
	if (outdevname.isEmpty() || (outdev = findDevice(outdevname)) < 0
			|| outChannels(outdev) <= 0)
		outdev = defaultOutputDevice();

#if 0
	QStringList inlist, outlist;
	for (int i = 0; i < ndevs; i++) {
		if (inChannels(dev) > 0)
			inlist.append(deviceName(i));
		if (outChannels(dev) > 0)
			outlist.append(deviceName(i));
	}
	inmod.setStringList(inlist);
	outmod.setStringList(outlist);
#endif

	return ndevs;
}

QString Audio::deviceName(int dev)
{
	return QString::fromLocal8Bit(Pa_GetDeviceInfo(dev)->name);
}

QStringList Audio::deviceNames()
{
	QStringList names;
	for (int i = 0; i < ndevs; i++)
		names.append(deviceName(i));
	return names;
}

QAbstractItemModel *Audio::inputDeviceListModel()
{
	return NULL;	//XXX
}

QAbstractItemModel *Audio::outputDeviceListModel()
{
	return NULL;	//XXX
}

int Audio::findDevice(const QString &name)
{
	for (int i = 0; i < ndevs; i++)
		if (deviceName(i) == name)
			return i;
	return -1;
}

int Audio::defaultInputDevice()
{
	return Pa_GetDefaultInputDeviceID();
}

int Audio::defaultOutputDevice()
{
	return Pa_GetDefaultOutputDeviceID();
}

void Audio::setInputDevice(int dev)
{
	Q_ASSERT(dev >= 0 && dev < ndevs && inChannels(dev) > 0);
	close();
	indev = dev;
	open();
}

void Audio::setOutputDevice(int dev)
{
	Q_ASSERT(dev >= 0 && dev < ndevs && outChannels(dev) > 0);
	close();
	outdev = dev;
	open();
}

void Audio::setInputDevice(const QString &name)
{
	for (int i = 0; i < Audio::numDevices(); i++) {
		if (deviceName(i) == name && inChannels(i) > 0) {
			setInputDevice(i);
			return;
		}
	}
	qWarning("Audio: unknown input device %s", name.toLocal8Bit().data());
}

void Audio::setOutputDevice(const QString &name)
{
	for (int i = 0; i < Audio::numDevices(); i++) {
		if (deviceName(i) == name && outChannels(i) > 0) {
			setOutputDevice(i);
			return;
		}
	}
	qWarning("Audio: unknown output device %s", name.toLocal8Bit().data());
}

int Audio::inChannels(int dev)
{
	return Pa_GetDeviceInfo(dev)->maxInputChannels;
}

int Audio::outChannels(int dev)
{
	return Pa_GetDeviceInfo(dev)->maxOutputChannels;
}

double Audio::minSampleRate(int dev)
{
	const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);
	Q_ASSERT(info->numSampleRates != 0);
	if (info->numSampleRates > 0) {
		double minrate = info->sampleRates[0];
		for (int i = 1; i < info->numSampleRates; i++)
			minrate = qMin(minrate, info->sampleRates[i]);
		return minrate;
	} else {
		Q_ASSERT(info->sampleRates[0] < info->sampleRates[1]);
		return info->sampleRates[0];
	}
}

double Audio::maxSampleRate(int dev)
{
	const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);
	Q_ASSERT(info->numSampleRates != 0);
	if (info->numSampleRates > 0) {
		double maxrate = info->sampleRates[0];
		for (int i = 1; i < info->numSampleRates; i++)
			maxrate = qMax(maxrate, info->sampleRates[i]);
		return maxrate;
	} else {
		Q_ASSERT(info->sampleRates[0] < info->sampleRates[1]);
		return info->sampleRates[1];
	}
}

void Audio::open()
{
	if (pas)
		return;	// Already open

	// Make sure we're initialized
	scan();

	// See if any input or output streams are actually enabled
	bool inena = !instreams.isEmpty();
	bool outena = !outstreams.isEmpty();
	if (!inena && !outena)
		return;

	// Use the maximum rate requested by any of our streams,
	// and the minimum framesize requested by any of our streams,
	// to maximize quality and minimize buffering latency.
	double maxrate = 0;
	int minframesize = 65536;
	for (int i = 0; i < instreams.size(); i++) {
		maxrate = qMax(maxrate, instreams[i]->rate);
		minframesize = qMin(minframesize, instreams[i]->framesize);
	}
	for (int i = 0; i < outstreams.size(); i++) {
		maxrate = qMax(maxrate, outstreams[i]->rate);
		minframesize = qMin(minframesize, outstreams[i]->framesize);
	}
	Q_ASSERT(maxrate > 0);

	// XXX check against rates supported by devices,
	// resample if necessary...

	qDebug() << "Open audio:"
		<< "input:" << indev << (inena ? "enable" : "disable")
		<< "output:" << outdev << (outena ? "enable" : "disable");

	// Open the audio device
	int rc = Pa_OpenStream(&pas,
			inena ? indev : paNoDevice, inena ? 1 : 0,
			paFloat32, NULL,
			outena ? outdev : paNoDevice, outena ? 1 : 0,
			paFloat32, NULL,
			maxrate, minframesize,
			Pa_GetMinNumBuffers(minframesize, maxrate)+10,
			paNoFlag, pacallback, NULL);
	if (rc) {
		qDebug() << "error" << rc;
		errmsg = Pa_GetErrorText(rc);
		qWarning("Error opening audio device: %s",
			Pa_GetErrorText(rc));
		return;
	}
	parate = maxrate;
	paframesize = minframesize;

	rc = Pa_StartStream(pas);
	if (rc) {
		errmsg = Pa_GetErrorText(rc);
		qWarning("Error starting audio stream: %s",
			Pa_GetErrorText(rc));
		return close();
	}
}

void Audio::reopen()
{
	qDebug() << "Audio::reopen";

	close();
	open();
}

void Audio::close()
{
	if (!pas)
		return;

	qDebug() << "Close audio stream";
	int rc = Pa_CloseStream(pas);
	if (rc)
		qWarning("Error closing audio stream: %s",
			Pa_GetErrorText(rc));
	pas = NULL;

	setInputLevel(0);
	setOutputLevel(0);
}

int Audio::rescan()
{
	close();
	if (ndevs >= 0) {
		Pa_Terminate();
		ndevs = -1;
	}

	int rc = scan();
	open();

	return rc;
}

int Audio::pacallback(void *inbuf, void *outbuf,
			unsigned long framesPerBuffer, PaTimestamp, void *)
{
	// A PortAudio "frame" is one sample per channel,
	// whereas our "frame" is one buffer worth of data (as in Speex).
	Q_ASSERT((int)framesPerBuffer == paframesize);

	if (inbuf != NULL)
		sendin((float*)inbuf);
	if (outbuf != NULL)
		mixout((float*)outbuf);

	return 0;
}

void Audio::sendin(const float *inbuf)
{
	// Broadcast the audio input to all listening input streams
	for (int i = 0; i < instreams.size(); i++) {
		AbstractAudioInput *ins = instreams[i];
		if (ins->rate == parate && ins->framesize == paframesize) {
			// The easy case - no buffering or resampling needed.
			ins->acceptInput(inbuf);
			continue;
		}

		Q_ASSERT(0);	// XXX
	}
}

void Audio::mixout(float *outbuf)
{
	int nout = outstreams.size();
	static int oldnout;
	if (nout != oldnout)
		qDebug() << "Audio:" << (oldnout = nout) << "output streams";

	if (nout == 0) {	// Nothing playing
		for (int i = 0; i < paframesize; i++)
			outbuf[i] = 0.0;
		setOutputLevel(0);
		return;
	}

	// Produce the first output stream's data directly into outbuf.
	outstreams[0]->getOutput(outbuf);

	// Mix any other streams into it.
	float encbuf[paframesize];
	for (int i = 1; i < nout; i++) {
		outstreams[i]->getOutput(encbuf);
		for (int j = 0; j < paframesize; j++)
			outbuf[j] += encbuf[j];
	}

	// Compute the output level
	Audio *me = instance();
	if (me->receivers(SIGNAL(outputLevelChanged(int))) > 0)
		me->setOutputLevel(computeLevel(outbuf));
}

int Audio::computeLevel(const float *buf)
{
	float lev = 0.0;
	for (int i = 0; i < paframesize; i++) {
		float l = buf[i];
		//lev = qMax(lev, l >= 0 ? l : -l);
		lev = qMax(lev, qAbs(l));
	}
	return (int)(lev * 100.0);
	return (int)((1.0 - log2f(lev)) * 100.0);
}

void Audio::setInputLevel(int lev)
{
	if (inlev == lev)
		return;

	inlev = lev;
	instance()->inputLevelChanged(lev);
}

void Audio::setOutputLevel(int lev)
{
	if (outlev == lev)
		return;

	outlev = lev;
	instance()->outputLevelChanged(lev);
}

void Audio::connectNotify(const char *signal)
{
	QObject::connectNotify(signal);
	if (QLatin1String(signal) == SIGNAL(inputLevelChanged(int))
			&& receivers(SIGNAL(inputLevelChanged(int))) > 0)
		inmon->enable();
}

void Audio::disconnectNotify(const char *signal)
{
	QObject::disconnectNotify(signal);
	if (QLatin1String(signal) == SIGNAL(inputLevelChanged(int))
			&& receivers(SIGNAL(inputLevelChanged(int)))  == 0)
		inmon->disable();
}

Audio::~Audio()
{
	close();
	if (ndevs >= 0)
		Pa_Terminate();
}


////////// AudioStream //////////

AudioStream::AudioStream(QObject *parent)
:	QObject(parent), enab(false), framesize(0), rate(0)
{
}

AudioStream::~AudioStream()
{
	disable();
}

void AudioStream::setEnabled(bool enable)
{
	enab = enable;
}

void AudioStream::setFrameSize(int framesize)
{
	Q_ASSERT(!enabled());
	this->framesize = framesize;
}

void AudioStream::setSampleRate(double samplerate)
{
	Q_ASSERT(!enabled());
	this->rate = samplerate;
}


////////// AbstractAudioInput //////////

AbstractAudioInput::AbstractAudioInput(QObject *parent)
:	AudioStream(parent)
{
}

AbstractAudioInput::AbstractAudioInput(int framesize, double samplerate,
					QObject *parent)
:	AudioStream(parent)
{
	setFrameSize(framesize);
	setSampleRate(samplerate);
}

void AbstractAudioInput::setEnabled(bool enable)
{
	if (enable && !enabled()) {
		if (frameSize() <= 0 || sampleRate() <= 0) {
			qWarning("AudioInput: bad frame size or sample rate");
			return;
		}

		Q_ASSERT(!Audio::instreams.contains(this));
		bool wasempty = Audio::instreams.isEmpty();
		Audio::instreams.append(this);
		AudioStream::setEnabled(true);
		if (wasempty || parate < sampleRate())
			Audio::reopen(); // (re-)open at suitable rate

	} else if (!enable && enabled()) {
		Q_ASSERT(Audio::instreams.contains(this));
		Audio::instreams.removeAll(this);
		AudioStream::setEnabled(false);
		if (Audio::instreams.isEmpty())
			Audio::reopen(); // close or reopen without input
	}
}


////////// AbstractAudioOutput //////////

AbstractAudioOutput::AbstractAudioOutput(QObject *parent)
:	AudioStream(parent)
{
}

AbstractAudioOutput::AbstractAudioOutput(int framesize, double samplerate,
					QObject *parent)
:	AudioStream(parent)
{
	setFrameSize(framesize);
	setSampleRate(samplerate);
}

void AbstractAudioOutput::setEnabled(bool enable)
{
	if (enable && !enabled()) {
		if (frameSize() <= 0 || sampleRate() <= 0) {
			qWarning("AudioOutput: bad frame size or sample rate");
			return;
		}

		Q_ASSERT(!Audio::outstreams.contains(this));
		bool wasempty = Audio::outstreams.isEmpty();
		Audio::outstreams.append(this);
		AudioStream::setEnabled(true);
		if (wasempty || parate < sampleRate())
			Audio::reopen(); // (re-)open at suitable rate

	} else if (!enable && enabled()) {
		Q_ASSERT(Audio::outstreams.contains(this));
		Audio::outstreams.removeAll(this);
		AudioStream::setEnabled(false);
		if (Audio::outstreams.isEmpty())
			Audio::reopen(); // close or reopen without output
	}
}

void AbstractAudioOutput::getOutput(float *buf)
{
	Q_ASSERT(sampleRate() == parate);	// XXX
	Q_ASSERT(frameSize() == paframesize);
	produceOutput(buf);
}


////////// AudioInput //////////

AudioInput::AudioInput(QObject *parent)
:	AbstractAudioInput(parent)
{
}

AudioInput::AudioInput(int framesize, double samplerate, QObject *parent)
:	AbstractAudioInput(framesize, samplerate, parent)
{
}

AudioInput::~AudioInput()
{
	reset();
}

void AudioInput::setFrameSize(int framesize)
{
	AbstractAudioInput::setFrameSize(framesize);
	reset();
}

int AudioInput::readFramesAvailable()
{
	QMutexLocker lock(&mutex);
	return inq.size();
}

int AudioInput::readFrames(float *buf, int maxframes)
{
	Q_ASSERT(maxframes >= 0);

	QMutexLocker lock(&mutex);
	int nframes = qMin(maxframes, inq.size());
	readInto(buf, nframes);
	return nframes;
}

QVector<float> AudioInput::readFrames(int maxframes)
{
	Q_ASSERT(maxframes >= 0);

	QMutexLocker lock(&mutex);
	int nframes = qMin(maxframes, inq.size());
	QVector<float> ret;
	ret.resize(nframes * frameSize());
	readInto(ret.data(), nframes);
	return ret;
}

void AudioInput::readInto(float *buf, int nframes)
{
	int framesize = frameSize();
	for (int i = 0; i < nframes; i++) {
		float *frame = inq.dequeue();
		memcpy(buf, frame, framesize * sizeof(float));
		delete [] frame;
		buf += framesize;
	}
}

void AudioInput::acceptInput(const float *buf)
{
	// Copy the input frame into a buffer for queueing
	int framesize = frameSize();
	float *frame = new float[framesize];
	memcpy(frame, buf, framesize * sizeof(float));

	// Queue the buffer
	mutex.lock();
	bool wasempty = inq.isEmpty();
	inq.enqueue(frame);
	mutex.unlock();

	// Notify reader if appropriate
	if (wasempty)
		readyRead();
}

void AudioInput::reset()
{
	disable();

	while (!inq.isEmpty())
		delete [] inq.dequeue();
}


////////// AudioOutput //////////

AudioOutput::AudioOutput(QObject *parent)
:	AbstractAudioOutput(parent)
{
}

AudioOutput::AudioOutput(int framesize, double samplerate, QObject *parent)
:	AbstractAudioOutput(framesize, samplerate, parent)
{
}

AudioOutput::~AudioOutput()
{
	reset();
}

void AudioOutput::setFrameSize(int framesize)
{
	AudioStream::setFrameSize(framesize);
	reset();
}

int AudioOutput::numFramesQueued()
{
	QMutexLocker lock(&mutex);
	return outq.size();
}

int AudioOutput::writeFrames(const float *buf, int nframes)
{
	int framesize = frameSize();
	QMutexLocker lock(&mutex);
	for (int i = 0; i < nframes; i++) {
		float *frame = new float[framesize];
		memcpy(frame, buf, framesize * sizeof(float));
		outq.enqueue(frame);
		buf += framesize;
	}
	return nframes;
}

int AudioOutput::writeFrames(const QVector<float> &buf)
{
	Q_ASSERT(buf.size() % frameSize() == 0);
	int nframes = buf.size() / frameSize();
	return writeFrames(buf.constData(), nframes);
}

void AudioOutput::produceOutput(float *buf)
{
	int framesize = frameSize();

	mutex.lock();
	bool emptied;
	if (outq.isEmpty()) {
		memset(buf, 0, framesize * sizeof(float));
		emptied = false;
	} else {
		float *frame = outq.dequeue();
		memcpy(buf, frame, framesize * sizeof(float));
		delete [] frame;
		emptied = outq.isEmpty();
	}
	mutex.unlock();

	if (emptied)
		queueEmpty();
}

void AudioOutput::reset()
{
	disable();

	while (!outq.isEmpty())
		delete [] outq.dequeue();
}


////////// AudioLoop //////////

AudioLoop::AudioLoop(QObject *parent)
:	AudioStream(parent),
	delay(5.0)
{
	connect(&in, SIGNAL(readyRead()), this, SLOT(inReadyRead()));
}

void AudioLoop::setLoopDelay(float secs)
{
	Q_ASSERT(!enabled());
	Q_ASSERT(secs > 0);
	delay = secs;
}

void AudioLoop::setEnabled(bool enable)
{
	if (enable && !enabled()) {
		int framesize = frameSize() ? frameSize() : monFrameSize;
		in.setFrameSize(framesize);
		out.setFrameSize(framesize);

		int samplerate = sampleRate() ? sampleRate() : monSampleRate;
		in.setSampleRate(samplerate);
		out.setSampleRate(samplerate);

		// Enable output when we have enough audio queued
		thresh = (int)(delay * in.sampleRate() / in.frameSize());

		in.enable();

	} else if (!enable && enabled()) {
		in.reset();
		out.reset();
	}
	AudioStream::setEnabled(enable);
}

void AudioLoop::inReadyRead()
{
	out.writeFrames(in.readFrames());
	if (!out.enabled() && out.numFramesQueued() >= thresh) {
		out.enable();
		startPlayback();
	}
}


////////// AudioInputMonitor //////////

AudioInputMonitor::AudioInputMonitor()
{
}

void AudioInputMonitor::acceptInput(const float *buf)
{
	// Compute the audio input level
	Audio::setInputLevel(Audio::computeLevel(buf));
}

