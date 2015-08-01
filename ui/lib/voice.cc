//=====================================================================================================================
// VoiceService
//=====================================================================================================================

static PacketInput* makeInputInstance(QObject* parent)
{
	return new RawInput(parent);
}

static PacketOutput* makeOutputInstance(QObject* parent)
{
	return new RawOutput(parent); //FileLoopedOutput("/Users/berkus/Hobby/mettanode/ui/gui/sounds/bnb.s16", parent);
}

VoiceService::VoiceService(QObject *parent)
	: PeerService("metta:Voice", tr("Voice communication"),
		"NodeVoice", tr("MettaNode voice communication protocol"), parent)
	, talkcol(-1)
	, lisncol(-1)
{
	vin = makeInputInstance(this);
	connect(vin, SIGNAL(readyRead()),
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
		vin->enable();

	} else {
		Stream *stream = outStream(hostid);
		if (!stream)
			return;
		sending.remove(stream);

		// Turn off audio input if we're no longer sending to anyone
		if (sending.isEmpty())
			vin->disable();
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

// void LiveMediaService::gotInStreamConnected(Stream *strm)
// {
	// We get a signaling stream incoming.
	// Media substreams may be demuxed into VoiceService, VideoService, ScreenShareService etc.
	// qDebug() << "LiveMediaService: incoming connection from" << peerName(strm->remoteHostId());
// }

void VoiceService::gotInStreamConnected(Stream *strm)
{
	if (!strm->isConnected())
		return;

	qDebug() << "VoiceService: incoming connection from" << peerName(strm->remoteHostId());

	ReceiveStream &rs = recv[strm];
	if (rs.stream != NULL)
		return;

	rs.stream = strm;
	rs.vout = makeOutputInstance(this); // @todo depend on negotiated payload type?
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
		QByteArray frame = vin->readFrame();
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
	PacketOutput *vout = static_cast<PacketOutput*>(sender());

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
	bool online = stream and stream->isConnected();

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


