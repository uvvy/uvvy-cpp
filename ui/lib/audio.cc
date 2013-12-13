
const int AbstractAudioInput::nChannels;
const int AbstractAudioOutput::nChannels;

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

static RtAudio *audio_inst;
static double hwrate;
static unsigned int hwframesize;

static int inlev, outlev;

static AudioInputMonitor *inmon;


static const int monSampleRate = 48000;
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

	try {
		audio_inst  = new RtAudio();
  	}
  	catch (RtError &error) {
		qWarning("Can't initialize RtAudio library, %s", error.what());
		return -1;
	}

	ndevs = audio_inst->getDeviceCount();
	if (ndevs == 0)
		errmsg = tr("No audio devices available");

	if (indevname.isEmpty() || (indev = findDevice(indevname)) < 0
			|| inChannels(indev) <= 0)
		indev = defaultInputDevice();
	if (outdevname.isEmpty() || (outdev = findDevice(outdevname)) < 0
			|| outChannels(outdev) <= 0)
		outdev = defaultOutputDevice();

	qDebug() << "Audio::scan:" << ndevs << "devices found.";

	int n = 0;
	foreach(QString s, deviceNames())
	{
		qDebug() << "Device" << n << ":" << s;
		++n;
	}

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
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d name: %s", dev, error.what());
      return QString();
    }
	return QString::fromLocal8Bit(info.name.c_str());
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
	return 0;
}

int Audio::defaultOutputDevice()
{
	return 2;
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
	qWarning() << "Audio: unknown input device" << name;
}

void Audio::setOutputDevice(const QString &name)
{
	for (int i = 0; i < Audio::numDevices(); i++) {
		if (deviceName(i) == name && outChannels(i) > 0) {
			setOutputDevice(i);
			return;
		}
	}
	qWarning() << "Audio: unknown output device" << name;
}

int Audio::inChannels(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d in channels: %s", dev, error.what());
      return 0;
    }
	return info.inputChannels;
}

int Audio::outChannels(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d out channels: %s", dev, error.what());
      return 0;
    }
	return info.outputChannels;
}

double Audio::minSampleRate(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d sample rates: %s", dev, error.what());
      return 0.0;
    }
	Q_ASSERT(info.sampleRates.size() != 0);
	int minrate = INT_MAX;
	foreach (int rate, info.sampleRates)
		minrate = qMin(minrate, rate);
	return minrate;
}

double Audio::maxSampleRate(int dev)
{
	RtAudio::DeviceInfo info;
    try {
      info = audio_inst->getDeviceInfo(dev);
    }
    catch (RtError &error) {
      qWarning("Error getting device %d sample rates: %s", dev, error.what());
      return 0.0;
    }
	Q_ASSERT(info.sampleRates.size() != 0);
	int maxrate = 0;
	foreach (int rate, info.sampleRates)
		maxrate = qMax(maxrate, rate);
	return maxrate;
}

void Audio::open()
{
	if (audio_inst->isStreamOpen())
		return;	// Already open

	// Make sure we're initialized
	scan();

	// See if any input or output streams are actually enabled
	bool inena = !instreams.isEmpty();
	bool outena = !outstreams.isEmpty();
	if (!inena && !outena)
		return;

	qDebug() << "In streams:" << instreams.size() << " Out streams:" << outstreams.size();

	// Use the maximum rate requested by any of our streams,
	// and the minimum framesize requested by any of our streams,
	// to maximize quality and minimize buffering latency.
	double maxrate = 0;
	unsigned int minframesize = 65536;
	for (int i = 0; i < instreams.size(); i++) {
		maxrate = qMax(maxrate, instreams[i]->sampleRate());
		minframesize = qMin(minframesize, instreams[i]->frameSize());
	}
	for (int i = 0; i < outstreams.size(); i++) {
		maxrate = qMax(maxrate, outstreams[i]->sampleRate());
		minframesize = qMin(minframesize, outstreams[i]->frameSize());
	}
	Q_ASSERT(maxrate > 0);
	Q_ASSERT(minframesize < 65536);

	// XXX check against rates supported by devices,
	// resample if necessary...

	qDebug() << "Open audio:"
		<< "input:" << indev << deviceName(indev) << (inena ? "enable" : "disable")
		<< "; output:" << outdev << deviceName(outdev) << (outena ? "enable" : "disable");

	// Open the audio device
	RtAudio::StreamParameters inparam, outparam;
	inparam.deviceId = indev;
	inparam.nChannels = AudioInput::nChannels;
	outparam.deviceId = outdev;
	outparam.nChannels = AudioOutput::nChannels;
	unsigned int bufferFrames = minframesize;

	qDebug() << "Open input params: dev " << indev << " nChans " << inparam.nChannels << " rate " << maxrate << " minframesize " << minframesize;

	try {
		audio_inst->openStream(outena ? &outparam : NULL, inena ? &inparam : NULL, RTAUDIO_FLOAT32, maxrate, &bufferFrames, rtcallback);
	}
    catch (RtError &error) {
    	qWarning() << "Couldn't open stream" << error.what();
    	return;
    }

	hwrate = maxrate;
	hwframesize = bufferFrames;

	qDebug() << "Open resulting hwrate" << hwrate << "framesize" << hwframesize;

	audio_inst->startStream();
}

void Audio::reopen()
{
	qDebug() << "Audio::reopen";

	close();
	open();
}

void Audio::close()
{
	if (!audio_inst->isStreamOpen())
		return;

	qDebug() << "Close audio stream";
	try {
		audio_inst->closeStream();
	}
    catch (RtError &error) {
    	qWarning() << "Couldn't close stream" << error.what();
    }

	setInputLevel(0);
	setOutputLevel(0);
}

int Audio::rescan()
{
	close();
	if (ndevs >= 0) {
		delete audio_inst; audio_inst = 0;
		ndevs = -1;
	}

	int rc = scan();
	open();

	return rc;
}

int Audio::rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double, RtAudioStreamStatus, void *)
{
	// A PortAudio "frame" is one sample per channel,
	// whereas our "frame" is one buffer worth of data (as in Speex).
	Q_ASSERT(nFrames == hwframesize);

	qDebug() << "rtcallback, inputBuffer" << inputBuffer << ", outputBuffer" << outputBuffer << ", nframes" << nFrames;

	if (inputBuffer != NULL)
		sendin((float*)inputBuffer);
	if (outputBuffer != NULL)
		mixout((float*)outputBuffer);

	return 0;
}

void Audio::sendin(const float *inbuf)
{
	// Broadcast the audio input to all listening input streams
	for (int i = 0; i < instreams.size(); i++) {
		AbstractAudioInput *ins = instreams[i];
		if (ins->sampleRate() == hwrate && ins->frameSize() == hwframesize) {
			// The easy case - no buffering or resampling needed.
			qDebug() << "sendin captured signal";
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
		for (unsigned int i = 0; i < hwframesize; i++)
			outbuf[i] = 0.0;
		setOutputLevel(0);
		return;
	}

	qDebug() << "mixout playback signal";

	// @TODO provide synchronization, the output stream might've ceased to exist here.
	// virtual void AbstractAudioInput::setEnabled(bool) false 
	// setInputLevel 52 
	// virtual void AbstractAudioOutput::setEnabled(bool) false 
	// mixout playback signal 
	// Fatal: ASSERT failure in QList<T>::operator[]: "index out of range", file /usr/local/Cellar/qt/4.8.3/include/QtCore/qlist.h, line 477
	// Audio::reopen 

	// Produce the first output stream's data directly into outbuf.
	outstreams[0]->getOutput(outbuf);

	// Mix any other streams into it.
	float encbuf[hwframesize];
	for (int i = 1; i < nout; i++) {
		outstreams[i]->getOutput(encbuf);
		for (unsigned int j = 0; j < hwframesize; j++)
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
	for (unsigned int i = 0; i < hwframesize; i++) {
		float l = buf[i];
		lev = qMax(lev, qAbs(l));
	}
	if (lev > 1.0)
		qWarning() << "HEY, TOO HIGH SIGNAL LEVEL" << lev;

	return (int)(lev * 100.0);
}

void Audio::setInputLevel(int lev)
{
	if (inlev == lev)
		return;

	qDebug() << "setInputLevel" << lev;
	inlev = lev;
	instance()->inputLevelChanged(lev);
}

void Audio::setOutputLevel(int lev)
{
	if (outlev == lev)
		return;

	qDebug() << "setOutputLevel" << lev;
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
	{
		delete audio_inst; audio_inst = 0;
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

