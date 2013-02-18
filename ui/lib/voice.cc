#include <QMutexLocker>
#include <QtDebug>
#include "voice.h"
#include "xdr.h"

//=====================================================================================================================
// OpusInput
//=====================================================================================================================

OpusInput::OpusInput(QObject *parent)
:	AbstractAudioInput(parent),
	encstate(NULL)
{
}

void OpusInput::setEnabled(bool enabling)
{
	if (enabling && !enabled()) {
		Q_ASSERT(!encstate);
		int error = 0;
		encstate = opus_encoder_create(48000, nChannels, OPUS_APPLICATION_VOIP, &error);
		Q_ASSERT(encstate);
		Q_ASSERT(!error);

		int framesize, rate;
		opus_encoder_ctl(encstate, OPUS_GET_SAMPLE_RATE(&rate));
		framesize = rate / 50; // 20ms
		setFrameSize(framesize);
		setSampleRate(rate);
		qDebug() << "OpusInput: frame size" << framesize << "sample rate" << rate;

		opus_encoder_ctl(encstate, OPUS_SET_VBR(1));
		opus_encoder_ctl(encstate, OPUS_SET_BITRATE(OPUS_AUTO));
		opus_encoder_ctl(encstate, OPUS_SET_DTX(1));

		AbstractAudioInput::setEnabled(true);

	} else if (!enabling && enabled()) {

		AbstractAudioInput::setEnabled(false);

		Q_ASSERT(encstate);
		opus_encoder_destroy(encstate);
		encstate = NULL;
	}
}

void OpusInput::acceptInput(const float *samplebuf)
{
	// Encode the frame and write it into a QByteArray buffer
	QByteArray bytebuf;
	int maxbytes = 1024;//meh, any opus option to get this?
	bytebuf.resize(maxbytes);
	int nbytes = opus_encode_float(encstate, samplebuf, frameSize(), (unsigned char*)bytebuf.data(), bytebuf.size());
	Q_ASSERT(nbytes <= maxbytes);
	bytebuf.resize(nbytes);
	qWarning() << "Encoded frame size:" << nbytes;

	// Queue it to the main thread
	mutex.lock();
	bool wasempty = inqueue.isEmpty();
	inqueue.enqueue(bytebuf);
	mutex.unlock();

	// Signal the main thread if appropriate
	if (wasempty)
		readyRead();
}

QByteArray OpusInput::readFrame()
{
	QMutexLocker lock(&mutex);

	if (inqueue.isEmpty())
		return QByteArray();

	return inqueue.dequeue();
}

//=====================================================================================================================
// RawInput
//=====================================================================================================================

RawInput::RawInput(QObject *parent)
	: AbstractAudioInput(parent)
{
}

void RawInput::acceptInput(const float *samplebuf)
{
	// Trivial XDR-based encoding, for debugging.
	QByteArray bytebuf;
	SST::XdrStream xws(&bytebuf, QIODevice::WriteOnly);
	for (int i = 0; i < frameSize(); i++)
		xws << samplebuf[i];

	// Queue it to the main thread
	mutex.lock();
	bool wasempty = inqueue.isEmpty();
	inqueue.enqueue(bytebuf);
	mutex.unlock();

	// Signal the main thread if appropriate
	if (wasempty)
		readyRead();
}

//=====================================================================================================================
// OpusOutput
//=====================================================================================================================

const int OpusOutput::maxSkip;

OpusOutput::OpusOutput(QObject *parent)
:	AbstractAudioOutput(parent),
	decstate(NULL),
	outseq(0)
{
}

void OpusOutput::setEnabled(bool enabling)
{
	if (enabling && !enabled()) {
		Q_ASSERT(!decstate);
		int error = 0;
		decstate = opus_decoder_create(48000, nChannels, &error);
		Q_ASSERT(decstate);
		Q_ASSERT(!error);

		int framesize, rate;
		opus_decoder_ctl(decstate, OPUS_GET_SAMPLE_RATE(&rate));
		framesize = rate / 50; // 20ms
		setFrameSize(framesize);
		setSampleRate(rate);
		qDebug() << "OpusOutput: frame size" << framesize << "sample rate" << rate;

		AbstractAudioOutput::setEnabled(true);

	} else if (!enabling && enabled()) {

		AbstractAudioOutput::setEnabled(false);

		Q_ASSERT(decstate);
		opus_decoder_destroy(decstate);
		decstate = NULL;
	}
}

void OpusOutput::produceOutput(float *samplebuf)
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
		qWarning() << "Decode frame size:" << bytebuf.size();
		int len = opus_decode_float(decstate, (unsigned char*)bytebuf.data(), bytebuf.size(), samplebuf, frameSize(), /*decodeFEC:*/1);
		Q_ASSERT(len > 0);
		Q_ASSERT(len == frameSize());

		// Signal the main thread if the queue empties
		if (nowempty)
			queueEmpty();
	} else {
		// "decode" a missing frame
		int len = opus_decode_float(decstate, NULL, 0, samplebuf, frameSize(), /*decodeFEC:*/1);
		Q_ASSERT(len > 0);
	}
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

void OpusOutput::writeFrame(const QByteArray &buf, qint32 seqno, int queuemax)
{
	// Determine how many frames we missed.
	qint32 seqdiff = seqno - outseq;
	if (seqdiff < 0) {
		// Out-of-order frame - just drop it for now.
		// XXX insert into queue out-of-order if it's still useful
		qDebug() << "OpusOutput: frame received out of order";
		return;
	}
	seqdiff = qMin(seqdiff, maxSkip);

	QMutexLocker lock(&mutex);

	// Queue up the missed frames, if any.
	for (int i = 0; i < seqdiff; i++) {
		outqueue.enqueue(QByteArray());
		qDebug() << "  MISSED audio frame" << outseq + i;
	}

	// Queue up the frame we actually got.
	outqueue.enqueue(buf);
	qDebug() << "Received audio frame" << seqno;

	// Discard frames from the head if we exceed queueMax
	while (outqueue.size() > queuemax)
		outqueue.removeFirst();

	// Remember which sequence we expect next
	outseq = seqno+1;
}

int OpusOutput::numFramesQueued()
{
	QMutexLocker lock(&mutex);
	return outqueue.size();
}

void OpusOutput::reset()
{
	disable();

	QMutexLocker lock(&mutex);
	outqueue.clear();
}

//=====================================================================================================================
// VoiceService
//=====================================================================================================================

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

void VoiceService::setTalkEnabled(const SST::PeerId& hostid, bool enable)
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

void VoiceService::setListenEnabled(const SST::PeerId& hostid, bool enable)
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

	qDebug() << "VoiceService: incoming connection from" << peerName(strm->remoteHostId());

	ReceiveStream &rs = recv[strm];
	if (rs.stream != NULL)
		return;

	rs.stream = strm;
	rs.vout = new OpusOutput(this);
	connect(strm, SIGNAL(readyReadDatagram()),
		this, SLOT(readyReadDatagram()));
	connect(rs.vout, SIGNAL(queueEmpty()),
		this, SLOT(voutQueueEmpty()));

	qDebug() << "VoiceService: established incoming connection from" << peerName(strm->remoteHostId());
}

void VoiceService::gotInStreamDisconnected(Stream *strm)
{
	qDebug() << "VoiceService: disconnection notification from" << peerName(strm->remoteHostId());
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
			Q_ASSERT(strm and ss.stream == strm);

			qDebug() << "Sending audio frame" << ss.seqno;

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
		qDebug() << "Received audio frame" << seqno;

		// Queue it to the audio system
		int nqueued = rs.vout->numFramesQueued();
		rs.vout->writeFrame(msg.mid(4), seqno, queueMax);

		// If the stream isn't enabled yet,
		// enable it once we get a threshold of frames queued
		if (nqueued >= queueMin) {
			qDebug() << "Enabling audio output";
			rs.vout->enable();
		}
	}
}

void VoiceService::voutQueueEmpty()
{
	OpusOutput *vout = (OpusOutput*)sender();

	foreach (Stream *strm, recv.keys()) {
		ReceiveStream &rs = recv[strm];
		if (rs.vout != vout)
			continue;

		qDebug() << "Disabling audio output";
		rs.vout->disable();
	}
}

void VoiceService::updateStatus(const SST::PeerId& id)
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
				? (talkoff.isNull() ? tr("Off") : talkoff)
			: sending.contains(stream)
				? (talkena.isNull() ? tr("Talking") : talkena)
				: (talkdis.isNull() ? tr("Muted") : talkdis);
		QModelIndex idx = ptab->index(row, talkcol);
		ptab->setData(idx, val, Qt::DisplayRole);
		ptab->setFlags(idx, Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	}

	if (lisncol >= 0) {
		// XXX
		QVariant val = tr("Off");
		QModelIndex idx = ptab->index(row, lisncol);
		ptab->setData(idx, val, Qt::DisplayRole);
		ptab->setFlags(idx, Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	}
}


