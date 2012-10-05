
#include <QMutexLocker>
#include <QtDebug>

#include "voice.h"
#include "xdr.h"


////////// SpeexInput //////////

SpeexInput::SpeexInput(QObject *parent)
:	AbstractAudioInput(parent),
	mod(NarrowBand),
	encstate(NULL)
{
}

void SpeexInput::setEnabled(bool enable)
{
	if (enable && !enabled()) {
		Q_ASSERT(!encstate);
		speex_bits_init(&bits);
		encstate = speex_encoder_init(modeptr(mod));
		Q_ASSERT(encstate);

		int framesize, rate;
		speex_encoder_ctl(encstate, SPEEX_GET_FRAME_SIZE, &framesize);
		speex_encoder_ctl(encstate, SPEEX_GET_SAMPLING_RATE, &rate);
		setFrameSize(framesize);
		setSampleRate(rate);
		qDebug() << "SpeexInput: frame size" << framesize
			<< "sample rate" << rate;

		int vbr = 1;
		speex_encoder_ctl(encstate, SPEEX_SET_VBR, &vbr);
		//int bitrate = 10000;
		//speex_encoder_ctl(encstate, SPEEX_SET_BITRATE, &bitrate);
		//int vad = 1;
		//speex_encoder_ctl(encstate, SPEEX_SET_VAD, &vad);

		AbstractAudioInput::setEnabled(true);

	} else if (!enable && enabled()) {

		AbstractAudioInput::setEnabled(false);

		Q_ASSERT(encstate);
		speex_bits_destroy(&bits);
		speex_encoder_destroy(encstate);
		encstate = NULL;
	}
}

void SpeexInput::acceptInput(const float *samplebuf)
{
#if 1
	// Speex uses a range of +/- 32767.0; PortAudio uses +/- 1.0.
	int nsamp = frameSize();
	float newbuf[nsamp];
	for (int i = 0; i < nsamp; i++)
		newbuf[i]  = samplebuf[i] * 32767.0;

	// Encode the frame
	speex_bits_reset(&bits);
	speex_encode(encstate, (float*)newbuf, &bits);

	// Write it into a QByteArray buffer
	QByteArray bytebuf;
	int maxbytes = speex_bits_nbytes(&bits);
	bytebuf.resize(maxbytes);
	int nbytes = speex_bits_write(&bits, bytebuf.data(), maxbytes);
	Q_ASSERT(nbytes <= maxbytes);
	bytebuf.resize(nbytes);
	//qDebug() << "Encoded frame:" << nbytes;
#else
	// Trivial XDR-based encoding, for debugging
	QByteArray bytebuf;
	XdrStream xws(&bytebuf, QIODevice::WriteOnly);
	for (int i = 0; i < frameSize(); i++)
		xws << samplebuf[i];
#endif

	// Queue it to the main thread
	mutex.lock();
	bool wasempty = inqueue.isEmpty();
	inqueue.enqueue(bytebuf);
	mutex.unlock();

	// Signal the main thread if appropriate
	if (wasempty)
		readyRead();
}

QByteArray SpeexInput::readFrame()
{
	QMutexLocker lock(&mutex);

	if (inqueue.isEmpty())
		return QByteArray();

	return inqueue.dequeue();
}


////////// SpeexOutput //////////

const int SpeexOutput::maxSkip;

SpeexOutput::SpeexOutput(QObject *parent)
:	AbstractAudioOutput(parent),
	mod(NarrowBand),
	decstate(NULL),
	outseq(0)
{
}

void SpeexOutput::setEnabled(bool enable)
{
	if (enable && !enabled()) {
		Q_ASSERT(!decstate);
		speex_bits_init(&bits);
		decstate = speex_decoder_init(modeptr(mod));
		Q_ASSERT(decstate);

		int framesize, rate;
		speex_decoder_ctl(decstate, SPEEX_GET_FRAME_SIZE, &framesize);
		speex_decoder_ctl(decstate, SPEEX_GET_SAMPLING_RATE, &rate);
		setFrameSize(framesize);
		setSampleRate(rate);
		qDebug() << "SpeexOutput: frame size" << framesize
			<< "sample rate" << rate;

		AbstractAudioOutput::setEnabled(true);

	} else if (!enable && enabled()) {

		AbstractAudioOutput::setEnabled(false);

		Q_ASSERT(decstate);
		speex_bits_destroy(&bits);
		speex_decoder_destroy(decstate);
		decstate = NULL;
	}
}

void SpeexOutput::produceOutput(float *samplebuf)
{
	// Grab the next buffer from the queue
	QByteArray bytebuf;
	mutex.lock();
	if (!outqueue.isEmpty())
		bytebuf = outqueue.dequeue();
	bool nowempty = outqueue.isEmpty();
	mutex.unlock();

#if 1
	// Decode the frame
	if (!bytebuf.isEmpty()) {
		//qDebug() << "Decode frame:" << bytebuf.size();
		speex_bits_reset(&bits);
		speex_bits_read_from(&bits, bytebuf.data(), bytebuf.size());
		speex_decode(decstate, &bits, samplebuf);

		// Signal the main thread if the queue empties
		if (nowempty)
			queueEmpty();
	} else {
		// "decode" a missing frame
		speex_decode(decstate, NULL, samplebuf);
	}

	// Speex uses a range of +/- 32767.0; PortAudio uses +/- 1.0.
	for (int i = 0; i < frameSize(); i++)
		samplebuf[i] /= 32767.0;
#else
	// Trivial XDR-based encoding, for debugging
	if (!bytebuf.isEmpty()) {
		XdrStream xrs(&bytebuf, QIODevice::ReadOnly);
		for (int i = 0; i < frameSize(); i++)
			xrs >> samplebuf[i];

		// Signal the main thread if the queue empties
		if (nowempty)
			queueEmpty();
	} else {
		memset(samplebuf, 0, frameSize() * sizeof(float));
	}
#endif
}

void SpeexOutput::writeFrame(const QByteArray &buf, qint32 seqno, int queuemax)
{
	// Determine how many frames we missed.
	qint32 seqdiff = seqno - outseq;
	if (seqdiff < 0) {
		// Out-of-order frame - just drop it for now.
		// XXX insert into queue out-of-order if it's still useful
		qDebug() << "SpeexOutput: frame received out of order";
		return;
	}
	seqdiff = qMin(seqdiff, maxSkip);

	QMutexLocker lock(&mutex);

	// Queue up the missed frames, if any.
	for (int i = 0; i < seqdiff; i++) {
		outqueue.enqueue(QByteArray());
		//qDebug() << "  MISSED audio frame" << outseq + i;
	}

	// Queue up the frame we actually got.
	outqueue.enqueue(buf);
	//qDebug() << "Received audio frame" << seqno;

	// Discard frames from the head if we exceed queueMax
	while (outqueue.size() > queuemax)
		outqueue.removeFirst();

	// Remember which sequence we expect next
	outseq = seqno+1;
}

int SpeexOutput::numFramesQueued()
{
	QMutexLocker lock(&mutex);
	return outqueue.size();
}

void SpeexOutput::reset()
{
	disable();

	QMutexLocker lock(&mutex);
	outqueue.clear();
}


////////// VoiceService //////////

VoiceService::VoiceService(QObject *parent)
:	PeerService("Voice", tr("Voice communication"),
		"NstVoice", tr("Netsteria voice communication protocol"),
		parent),
	talkcol(-1), lisncol(-1)
{
	connect(&vin, SIGNAL(readyRead()),
		this, SLOT(vinReadyRead()));
	connect(this, SIGNAL(outStreamDisconnected(Stream*)),
		this, SLOT(gotOutStreamDisconnected(Stream*)));
	connect(this, SIGNAL(inStreamConnected(Stream*)),
		this, SLOT(gotInStreamConnected(Stream*)));
	connect(this, SIGNAL(inStreamDisconnected(Stream*)),
		this, SLOT(gotInStreamDisconnected(Stream*)));
}

void VoiceService::setTalkEnabled(const QByteArray &hostid, bool enable)
{
	if (enable) {
		qDebug() << "VoiceService: talking to" << peerName(hostid);

		Stream *stream = connectToPeer(hostid);
		if (!stream->isConnected()) {
			reconnectToPeer(hostid);
			return;	// Can't talk unless connected
		}
		SendStream &ss = send[stream];	// Creates if doesn't exist
		ss.stream = stream;
		sending.insert(stream);

		// Make sure audio input is enabled
		vin.enable();

	} else {
		Stream *stream = outStream(hostid);
		if (!stream)
			return;
		sending.remove(stream);

		// Turn off audio input if we're no longer sending to anyone
		if (sending.isEmpty())
			vin.disable();
	}

	updateStatus(hostid);
}

void VoiceService::setListenEnabled(const QByteArray &hostid, bool enable)
{
	// XXX
}

void VoiceService::setTalkColumn(int column,
		const QVariant &enableValue,
		const QVariant &disableValue,
		const QVariant &offlineValue)
{
	talkcol = column;
	talkena = enableValue;
	talkdis = disableValue;
	talkoff = offlineValue;
	updateStatusAll();
}

void VoiceService::setListenColumn(int column,
		const QVariant &enableValue,
		const QVariant &disableValue)
{
	lisncol = column;
	lisnena = enableValue;
	lisndis = disableValue;
	updateStatusAll();
}

void VoiceService::gotOutStreamDisconnected(Stream *strm)
{
	setTalkEnabled(strm->remoteHostId(), false);

	// Clean up our state for this stream
	Q_ASSERT(!sending.contains(strm));
	send.remove(strm);
}

void VoiceService::gotInStreamConnected(Stream *strm)
{
	if (!strm->isConnected())
		return;

	qDebug() << "VoiceService: incoming connection from"
		<< peerName(strm->remoteHostId());

	ReceiveStream &rs = recv[strm];
	if (rs.stream != NULL)
		return;

	rs.stream = strm;
	rs.vout = new SpeexOutput(this);
	connect(strm, SIGNAL(readyReadDatagram()),
		this, SLOT(readyReadDatagram()));
	connect(rs.vout, SIGNAL(queueEmpty()),
		this, SLOT(voutQueueEmpty()));
}

void VoiceService::gotInStreamDisconnected(Stream *strm)
{
	ReceiveStream rs = recv.take(strm);
	if (rs.vout)
		delete rs.vout;
}

void VoiceService::vinReadyRead()
{
	forever {
		// Read a frame from the audio system
		QByteArray frame = vin.readFrame();
		if (frame.isEmpty())
			break;

		// Build a message template
		QByteArray msg;
		msg.resize(4);
		msg.append(frame);

		// Broadcast it to everyone we're talking to
		foreach (Stream *strm, sending) {
			SendStream &ss = send[strm];
			Q_ASSERT(strm && ss.stream == strm);

			//qDebug() << "Sending audio frame" << ss.seqno;

			// Set the message sequence number
			*(qint32*)msg.data() = htonl(ss.seqno++);

			// Ship it off
			strm->writeDatagram(msg, FALSE);
		}
	}
}

void VoiceService::readyReadDatagram()
{
	Stream *strm = (Stream*)sender();
	Q_ASSERT(strm);

	if (!strm->isConnected() || !recv.contains(strm))
		return;
	ReceiveStream &rs = recv[strm];
	Q_ASSERT(rs.stream == strm);
	Q_ASSERT(rs.vout != NULL);

	forever {
		QByteArray msg = strm->readDatagram();	// XX max size?
		if (msg.isEmpty())
			break;

		// Read the sequence number header
		if (msg.size() < 4)
			continue;
		qint32 seqno = ntohl(*(qint32*)msg.data());
		//qDebug() << "Received audio frame" << seqno;

		// Queue it to the audio system
		int nqueued = rs.vout->numFramesQueued();
		rs.vout->writeFrame(msg.mid(4), seqno, queueMax);

		// If the stream isn't enabled yet,
		// enable it once we get a threshold of frames queued
		if (nqueued >= queueMin) {
			//qDebug() << "Enabling audio output";
			rs.vout->enable();
		}
	}
}

void VoiceService::voutQueueEmpty()
{
	SpeexOutput *vout = (SpeexOutput*)sender();

	foreach (Stream *strm, recv.keys()) {
		ReceiveStream &rs = recv[strm];
		if (rs.vout != vout)
			continue;

		qDebug() << "Disabling audio output";
		rs.vout->disable();
	}
}

void VoiceService::updateStatus(const QByteArray &id)
{
	PeerService::updateStatus(id);

	PeerTable *ptab = peerTable();
	if (!ptab)
		return;
	int row = ptab->idRow(id);
	if (row < 0)
		return;

	Stream *stream = outStream(id);
	bool online = stream && stream->isConnected();

	if (talkcol >= 0) {
		QVariant val = !online
				? (talkoff.isNull() ? tr("Offline") : talkoff)
			: sending.contains(stream)
				? (talkena.isNull() ? tr("Talking") : talkena)
				: (talkdis.isNull() ? tr("Online") : talkdis);
		QModelIndex idx = ptab->index(row, talkcol);
		ptab->setData(idx, val, Qt::DisplayRole);
		ptab->setFlags(idx, Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	}

	if (lisncol >= 0) {
		// XXX
	}
}


