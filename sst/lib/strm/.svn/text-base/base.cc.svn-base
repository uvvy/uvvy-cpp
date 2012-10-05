
// XXX work around apparent bug in g++ 4.1.2
#include <QtGlobal>
namespace SST { struct UniqueStreamId; };
uint qHash(const SST::UniqueStreamId &usid);

#include <QtDebug>

#include "xdr.h"
#include "host.h"
#include "strm/base.h"
#include "strm/dgram.h"
#include "strm/peer.h"
#include "strm/sflow.h"

using namespace SST;


////////// StreamTxAttachment //////////

void StreamTxAttachment::setAttaching(StreamFlow *flow, quint16 sid)
{
	Q_ASSERT(!isInUse());
	this->flow = flow;
	this->sid = sid;
	this->sidseq = maxPacketSeq;	// set when we get Ack
	this->active = this->deprecated = false;

	Q_ASSERT(!flow->txsids.contains(sid));
	flow->txsids.insert(sid, this);
}

void StreamTxAttachment::clear()
{
	StreamFlow *fl = flow;
	if (!fl)
		return;

	if (strm->tcuratt == this)
		strm->tcuratt = NULL;	// XX any notification?

	Q_ASSERT(fl->txsids.value(sid) == this);
	fl->txsids.remove(sid);
	flow = NULL;
	active = false;

	// Remove the stream from the flow's waiting streams list
	int rc = fl->dequeueStream(strm);
	Q_ASSERT(rc <= 1);
	strm->tqflow = false;

	// Clear out packets for this stream from flow's ackwait table
	foreach (qint64 txseq, fl->ackwait.keys()) {
		BaseStream::Packet &p = fl->ackwait[txseq];
		Q_ASSERT(!p.isNull());
		Q_ASSERT(p.strm);
		if (p.strm != strm)
			continue;

		// Move the packet back to the stream's transmit queue
		if (!p.late) {
			p.late = true;
			strm->missed(fl, p);
		} else
			strm->expire(fl, p);
		fl->ackwait.remove(txseq);
	}
}


////////// StreamRxAttachment //////////

void StreamRxAttachment::setActive(
		StreamFlow *flow, quint16 sid, PacketSeq rxseq)
{
	Q_ASSERT(!isActive());
	this->flow = flow;
	this->sid = sid;
	this->sidseq = rxseq;

	Q_ASSERT(!flow->rxsids.contains(sid));
	flow->rxsids.insert(sid, this);
}

void StreamRxAttachment::clear()
{
	if (flow) {
		Q_ASSERT(flow->rxsids.value(sid) == this);
		flow->rxsids.remove(sid);
		flow = NULL;
	}
}


////////// BaseStream //////////

BaseStream::BaseStream(Host *h, QByteArray peerid, BaseStream *parent)
:	AbstractStream(h),
	parent(parent),
	state(Fresh),
	init(true),
	toplev(false),
	endread(false),
	endwrite(false),
	tcuratt(NULL),
	tasn(0), twin(0), tflt(0), tqflow(false), twaitsize(0),
	tswin(0), tsflt(0),
	rsn(0),
	ravail(0), rmsgavail(0), rbufused(0),
	rcvbuf(defaultReceiveBuffer),
	crcvbuf(defaultReceiveBuffer)
{
	// Initialize inherited parameters
	if (parent) {
		if (parent->listenMode() & Stream::Inherit)
			AbstractStream::listen(parent->listenMode());
		rcvbuf = crcvbuf = parent->crcvbuf;
	}

	calcReceiveWindow();	// initialize rwinbyte

	Q_ASSERT(!peerid.isEmpty());
	this->peerid = peerid;
	this->peer = h->streamPeer(peerid);

	// Insert us into the peer's master list of streams
	peer->allstreams.insert(this);

	// Initialize the stream back-pointers in the attachment slots.
	for (int i = 0; i < maxAttach; i++) {
		tatt[i].strm = this;
		ratt[i].strm = this;
	}
}

BaseStream::~BaseStream()
{
	//qDebug() << "~" << this << (parent == NULL ? "(root)" : "");
	clear();
}

void BaseStream::clear()
{
	state = Disconnected;
	endread = endwrite = true;

	// De-register us from our peer
	if (peer) {
		if (peer->usids.value(usid) == this)
			peer->usids.remove(usid);
		peer->allstreams.remove(this);
		peer = NULL;
	}

	// Clear any attachments we may have
	for (int i = 0; i < maxAttach; i++) {
		tatt[i].clear();
		ratt[i].clear();
	}

	// Reset any unaccepted incoming substreams too
	foreach (AbstractStream *sub, rsubs) {
		sub->shutdown(Stream::Reset);
		// should self-destruct automatically when done
	}
	rsubs.clear();
}

void BaseStream::connectTo(const QString &service, const QString &protocol)
{
	Q_ASSERT(!service.isEmpty());
	Q_ASSERT(state == Fresh);
	Q_ASSERT(tcuratt == NULL);

	// Find or create the Target struct for the destination ID
	toplev = true;

	// Queue up a service connect message onto the new stream.
	// This will only go out once we actually attach to a flow,
	// but the client can immediately enqueue application data behind it.
	QByteArray msg;
	XdrStream ws(&msg, QIODevice::WriteOnly);
	ws << (qint32)ConnectRequest << service << protocol;
	writeMessage(msg.data(), msg.size());

	// Record that we're waiting for a response from the server.
	state = WaitService;

	// Attach to a suitable flow, initiating a new one if necessary.
	tattach();
}

void BaseStream::tattach()
{
	Q_ASSERT(!peerid.isEmpty());

	// If we already have a transmit-attachment, nothing to do.
	if (tcuratt != NULL) {
		Q_ASSERT(tcuratt->isInUse());
		return;
	}

	// If we're disconnected, we'll never need to attach again...
	if (state == Disconnected)
		return;

	// See if there's already an active flow for this peer.
	// If so, use it - otherwise, create one.
	if (!peer->flow) {
		// Get the flow setup process for this host ID underway.
		// XX provide an initial packet to avoid an extra RTT!
		//qDebug() << this << "tattach: wait for flow";
		connect(peer, SIGNAL(flowConnected()),
			this, SLOT(gotFlowConnected()));
		return peer->connectFlow();
	}
	StreamFlow *flow = peer->flow;
	Q_ASSERT(flow->isActive());

	// If we're initiating a new stream and our peer hasn't acked it yet,
	// make sure we have a parent USID to refer to in creating the stream.
	if (init && pusid.isNull()) {
		// No parent USID yet - try to get it from the parent stream.
		if (!parent) {
			// Top-level streams just use any flow's root stream.
			if (toplev)
				parent = &flow->root;
			else
				return fail(tr("Parent stream closed before "
						"child could be initiated"));
		}
		pusid = parent->usid;
		if (pusid.isNull()) {
			// Parent itself doesn't have a USID yet -
			// we have to wait until it does.
			qDebug() << this << "parent has no USID yet: waiting";
			connect(parent, SIGNAL(attached()),
				this, SLOT(gotParentAttached()));
			return parent->tattach();
		}
	}

	// Allocate a StreamId for this stream.
	// Scan forward through our SID space a little ways for a free SID;
	// if there are none, then pick a random one and detach it.
	StreamCtr ctr = flow->txctr;
	if (flow->txsids.contains(ctr)) {
		int maxsearch = 0x7ff0 - (flow->txctr - flow->txackctr);
		maxsearch = qMin(maxsearch, maxSidSkip);
		do {
			if (maxsearch-- <= 0) {
				qDebug() << this << "tattach: no free SID";
				Q_ASSERT(0);	// XXX wait for a free one
			}
		} while (flow->txsids.contains(++ctr));
	}

	// Find a free attachment slot.
	int slot = 0;
	while (tatt[slot].isInUse()) {
		if (++slot == maxAttach) {
			// All attachment slots are still in use.
			// This probably should never happen in practice,
			// but could if all slots are trying to detach...
			qDebug() << this << "tattach: all slots in use";
			Q_ASSERT(0);	// XXX wait for a free one
		}
	}

	// Update our stream counter
	Q_ASSERT(ctr >= flow->txctr);
	flow->txctr = ctr+1;

	// Attach to the stream using the selected slot.
	tatt[slot].setAttaching(flow, ctr);
	tcuratt = &tatt[slot];

	// Fill in the new stream's USID, if it doesn't have one yet.
	if (usid.isNull()) {
		setUsid(UniqueStreamId(ctr, flow->txChannelId()));
		qDebug() << this << "creating stream" << usid;
	}

	// Get us in line to transmit on the flow.
	// We at least need to transmit an attach message of some kind;
	// in the case of Init or Reply it might also include data.
	Q_ASSERT(!flow->tstreams.contains(this));
	txenqflow();
	if (flow->mayTransmit())
		flow->readyTransmit();
}

void BaseStream::gotParentAttached()
{
	qDebug() << this << "gotParentAttached";
	if (parent)
		QObject::disconnect(parent, SIGNAL(attached()),
				this, SLOT(gotParentAttached()));

	// Retry attach now that parent hopefully has a USID
	return tattach();
}

void BaseStream::gotFlowConnected()
{
	qDebug() << this << "gotFlowConnected";
	if (peer)
		QObject::disconnect(peer, SIGNAL(flowConnected()),
				this, SLOT(gotFlowConnected()));

	// Retry attach now that we hopefully have an active flow
	tattach();
}

void BaseStream::setUsid(const UniqueStreamId &newusid)
{
	Q_ASSERT(usid.isNull());
	Q_ASSERT(!newusid.isNull());

	if (peer->usids.contains(usid))
		qWarning("Duplicate stream USID!?");

	usid = newusid;
	peer->usids.insert(usid, this);
}

void BaseStream::gotServiceReply()
{
	Q_ASSERT(state == WaitService);
	Q_ASSERT(tcuratt);

	QByteArray msg(readMessage(maxServiceMsgSize));
	XdrStream rs(&msg, QIODevice::ReadOnly);
	qint32 code, err;
	rs >> code >> err;
	if (rs.status() != rs.Ok || code != (qint32)ConnectReply || err)
		return fail(tr("Service connect failed: %0 %1")
					.arg(code).arg(err));	// XX

	state = Connected;
	if (strm)
		strm->linkUp();
}

void BaseStream::gotServiceRequest()
{
	Q_ASSERT(state == Accepting);

	QByteArray msg(readMessage(maxServiceMsgSize));
	XdrStream rs(&msg, QIODevice::ReadOnly);
	qint32 code;
	ServicePair svpair;
	rs >> code >> svpair.first >> svpair.second;
	if (rs.status() != rs.Ok || code != (qint32)ConnectRequest)
		return fail("Bad service request");

	qDebug() << this << "gotServiceRequest service" << svpair.first
		<< "protocol" << svpair.second;

	// Lookup the requested service
	StreamServer *svr = h->listeners.value(svpair);
	if (svr == NULL)
		// XXX send reply with error code/message
		return fail(
			tr("Request for service %0 with unknown protocol %1")
				.arg(svpair.first).arg(svpair.second));

	// Send a service reply to the client
	msg.clear();
	XdrStream ws(&msg, QIODevice::WriteOnly);
	ws << (qint32)ConnectReply << (qint32)0;
	writeMessage(msg.data(), msg.size());

	// Hand off the new stream to the chosen service
	state = Connected;
	svr->rconns.enqueue(this);
	svr->newConnection();
}

AbstractStream *BaseStream::openSubstream()
{
	// Create the new sub-BaseStream.
	// Note that the parent doesn't have to be attached yet:
	// the substream will attach and wait for the parent if necessary.
	BaseStream *nstrm = new BaseStream(h, peerid, this);
	nstrm->state = Connected;

	// Start trying to attach the new stream, if possible.
	tattach();

	return nstrm;
}

void BaseStream::setPriority(int newpri)
{
	//qDebug() << this << "set priority" << newpri;
	AbstractStream::setPriority(newpri);

	if (tqflow) {
		StreamFlow *flow = tcuratt->flow;
		Q_ASSERT(flow->isActive());
		int rc = flow->dequeueStream(this);
		Q_ASSERT(rc == 1);
		flow->enqueueStream(this);
	}
}

void BaseStream::txenqueue(const Packet &pkt)
{
	// Add the packet to our stream-local transmit queue.
	// Keep packets in order of transmit sequence number,
	// but in FIFO order for packets with the same sequence number.
	// This happens because datagram packets get assigned the current TSN
	// when they are queued, but without actually incrementing the TSN,
	// just to keep them in the right order with respect to segments.
	// (The assigned TSN is not transmitted in the datagram, of course).
	int i = tqueue.size();
	while (i > 0 && (tqueue[i-1].tsn - pkt.tsn) > 0)
		i--;
	tqueue.insert(i, pkt);

	// Add our stream to our flow's transmit queue
	txenqflow(true);
}

void BaseStream::txenqflow(bool immed)
{
	//qDebug() << this << "txenqflow" << immed;

	// Make sure we're attached to a flow - if not, attach.
	if (!tcuratt)
		return tattach();
	StreamFlow *flow = tcuratt->flow;
	Q_ASSERT(flow && flow->isActive());

	// Enqueue this stream to the flow's transmit queue.
	if (!tqflow) {
		if (tqueue.isEmpty()) {
			if (strm) // Nothing to transmit - prod application.
				strm->readyWrite();
		} else {
			flow->enqueueStream(this);
			tqflow = true;
		}
	}

	// Prod the flow to transmit immediately if possible
	if (immed && flow->mayTransmit())
		flow->readyTransmit();
}

// Called by StreamFlow::readyTransmit()
// to transmit or retransmit one packet on a given flow.
// That packet might have to be an attach packet
// if we haven't finished attaching yet,
// or it might have to be an empty segment
// if we've run out of transmit window space
// but our latest receive window update may be out-of-date.
void BaseStream::transmit(StreamFlow *flow)
{
	Q_ASSERT(tqflow);
	Q_ASSERT(tcuratt != NULL);
	Q_ASSERT(flow == tcuratt->flow);
	Q_ASSERT(!tqueue.isEmpty());

	tqflow = false;	// flow just dequeued us

	// First garbage-collect any segments that have already been ACKed;
	// this can happen if we retransmit a segment
	// but an ACK for the original arrives late.
	Packet *hp = &tqueue.head();
	while (hp->type == DataPacket && !twait.contains(hp->tsn)) {
		// No longer waiting for this tsn - must have been ACKed.
		tqueue.removeFirst();
		if (tqueue.isEmpty()) {
			if (strm)
				strm->readyWrite();
			return;
		}
		hp = &tqueue.head();
	}

	// Ensure our attachment has been acknowledged before using the SID.
	int segsize = hp->payloadSize();
	if (tcuratt->isAcked()) {
		// Our attachment has been acknowledged:
		// an ordinary data packet will do fine.
		Q_ASSERT(!init);
		Q_ASSERT(tcuratt->isActive());

		// Throttle data transmission if flow window is full
		if (tflt + segsize > twin) {
			qDebug() << this << "transmit window full:"
				<< "need" << (tflt + segsize)
				<< "have" << twin;
			// XXX query status if latest update is out-of-date!
			//XXXreturn;
		}

		// Datagrams get special handling.
		if (hp->type == DatagramPacket)
			return txDatagram();

		// Register the segment as being in-flight.
		tflt += hp->payloadSize();
		//qDebug() << this << "inflight Data" << hp->tsn
		//	<< "bytes in flight" << tflt;

		// Transmit the next segment in a regular Data packet.
		Packet p = tqueue.dequeue();
		Q_ASSERT(p.type == DataPacket);
		Q_ASSERT(hdrlenData == Flow::hdrlen + sizeof(DataHeader));

		DataHeader *hdr = (DataHeader*)(p.buf.data() + Flow::hdrlen);
		hdr->sid = htons(tcuratt->sid);
		hdr->type = (DataPacket << typeShift) |
				(hdr->type & dataAllFlags);
			// (flags already set - preserve)
		hdr->win = receiveWindow();
		hdr->tsn = htonl(p.tsn);		// Note: 32-bit TSN
		return txData(p);
	}

	// See if we can potentially use an optimized attach/data packet;
	// this only works for regular stream segments, not datagrams,
	// and only within the first 2^16 bytes of the stream.
	if (hp->type == DataPacket && hp->tsn <= 0xffff) {

		// See if we can attach stream using an optimized Init packet,
		// allowing us to indicate the parent with a short 16-bit LSID
		// and piggyback useful data onto the packet.
		// The parent must be attached to the same flow.
		// XX probably should use some state invariant
		// in place of all these checks.
		if (toplev)
			parent = &flow->root;
		if (init && parent && parent->tcuratt
				&& parent->tcuratt->flow == flow
				&& parent->tcuratt->isActive()
				&& usid.chanId == flow->txChannelId()
				&& (quint16)usid.streamCtr == tcuratt->sid
			/* XXX	&& parent->tflt + segsize <= parent->twin*/) {
			//qDebug() << "sending Init packet";

			// Adjust the in-flight byte count for flow control.
			// Init packets get "charged" to the parent stream.
			parent->tflt += hp->payloadSize();
			qDebug() << this << "inflight Init" << hp->tsn
				<< "bytes in flight (parent)" << parent->tflt;

			return txAttachData(InitPacket, parent->tcuratt->sid);
		}

		// See if our peer has this stream in its SID space,
		// allowing us to attach using an optimized Reply packet.
		if (tflt + segsize > twin)
			goto noReply;	// no room for data: just Attach
		for (int i = 0; i < maxAttach; i++) {
			if (ratt[i].flow == flow && ratt[i].isActive()) {
				//qDebug() << "sending Reply packet";

				// Adjust the in-flight byte count.
				tflt += hp->payloadSize();
				qDebug() << this << "inflight Reply" << hp->tsn
					<< "bytes in flight" << tflt;

				return txAttachData(ReplyPacket, ratt[i].sid);
			}
		}
		noReply: ;
	}

	// We've exhausted all of our optimized-path options:
	// we have to send a specialized Attach packet instead of useful data.
	txAttach();

	// Don't requeue onto our flow at this point -
	// we can't transmit any data until we get that ack!
}

void BaseStream::txAttachData(PacketType type, StreamId refsid)
{
	Packet p = tqueue.dequeue();
	Q_ASSERT(p.type == DataPacket);		// caller already checked
	Q_ASSERT(p.tsn <= 0xffff);		// caller already checked
	Q_ASSERT(hdrlenInit == Flow::hdrlen + sizeof(InitHeader));

	// Build the InitHeader.
	InitHeader *hdr = (InitHeader*)(p.buf.data() + Flow::hdrlen);
	hdr->sid = htons(tcuratt->sid);
	hdr->type = (type << typeShift) | (hdr->type & dataAllFlags);
			// (flags already set - preserve)
	hdr->win = receiveWindow();
	hdr->rsid = htons(refsid);
	hdr->tsn = htons(p.tsn);		// Note: 16-bit TSN

	// Transmit
	return txData(p);
}

void BaseStream::txData(Packet &p)
{
	StreamFlow *flow = tcuratt->flow;

	// Transmit the packet on our current flow.
	quint64 pktseq;
	flow->flowTransmit(p.buf, pktseq);
	Q_ASSERT(pktseq);	// XXX
	//qDebug() << strm << "tx " << pktseq
	//	<< "posn" << p.tsn << "size" << p.buf.size();

	// Save the data packet in the flow's ackwait hash.
	p.late = false;
	flow->ackwait.insert(pktseq, p);

	// Re-queue us on our flow immediately
	// if we still have more data to send.
	if (tqueue.isEmpty()) {
		if (strm)
			strm->readyWrite();
	} else
		txenqflow();
}

void BaseStream::txDatagram()
{
	//qDebug() << this << "txDatagram";

	// Transmit the whole the datagram immediately,
	// so that all fragments get consecutive packet sequence numbers.
	while (true) {
		Q_ASSERT(!tqueue.isEmpty());
		Packet p = tqueue.dequeue();
		Q_ASSERT(p.type == DatagramPacket);
		DatagramHeader *hdr = (DatagramHeader*)
						(p.buf.data() + Flow::hdrlen);
		qint8 atend = hdr->type & dgramEndFlag;
		hdr->sid = htons(tcuratt->sid);
		hdr->win = receiveWindow();

		// Adjust the in-flight byte count.
		// We don't need to register datagram packets in tflt
		// because we don't keep them around after they're "missed" -
		// which is fortunate since we _can't_ register them
		// because they don't have unique TSNs.
		tflt += p.payloadSize();

		// Transmit this datagram packet,
		// but don't save it anywhere - just fire & forget.
		quint64 pktseq;
		tcuratt->flow->flowTransmit(p.buf, pktseq);

		if (atend)
			break;
	}

	// Re-queue us on our flow immediately
	// if we still have more data to send.
	return txenqflow();
}

void BaseStream::txAttach()
{
	qDebug() << this << "transmit Attach packet";
	StreamFlow *flow = tcuratt->flow;
	Q_ASSERT(!usid.isNull());	// XXX am I sure this holds here?
	Q_ASSERT(!pusid.isNull());	// XXX am I sure this holds here?

	// What slot are we trying to attach with?
	unsigned slot = tcuratt - tatt;
	Q_ASSERT(slot < (unsigned)maxAttach);
	Q_ASSERT(attachSlotMask == maxAttach-1);

	// Build the Attach packet header
	Packet p(this, AttachPacket);
	p.buf.resize(hdrlenAttach);
	AttachHeader *hdr = (AttachHeader*)(p.buf.data() + Flow::hdrlen);
	hdr->sid = htons(tcuratt->sid);
	hdr->type = (AttachPacket << typeShift) | (init ? attachInitFlag : 0)				| slot;
	hdr->win = receiveWindow();

	// The body of the Attach packet is the stream's full USID,
	// and the parent's USID too if we're still initiating the stream.
	QByteArray bodybuf;
	XdrStream bodyxs(&bodybuf, QIODevice::WriteOnly);
	bodyxs << usid;
	if (init)
		bodyxs << pusid;
	p.buf.append(bodybuf);

	// Transmit it on the current flow.
	quint64 pktseq;
	flow->flowTransmit(p.buf, pktseq);

	// Save the attach packet in the flow's ackwait hash,
	// so that we'll be notified when the attach packet gets acked.
	p.late = false;
	flow->ackwait.insert(pktseq, p);
}

void BaseStream::txReset(StreamFlow */*flow*/, quint16 /*sid*/,
			quint8 /*flags*/)
{
	qWarning("XXX txReset NOT IMPLEMENTED YET!!!");
}

void BaseStream::acked(StreamFlow *flow, const Packet &pkt, quint64 rxseq)
{
	//qDebug() << "BaseStream::acked packet of size" << pkt.buf.size();

	switch (pkt.type) {
	case DataPacket:
		// Mark the segment no longer "in flight".
		endflight(pkt);

		// Record this TSN as having been ACKed (if not already),
		// so that we don't spuriously resend it
		// if another instance is back in our transmit queue.
		if (twait.remove(pkt.tsn)) {
			twaitsize -= pkt.payloadSize();
			//qDebug() << "twait remove" << pkt.tsn
			//	<< "size" << pkt.payloadSize()
			//	<< "new cnt" << twait.size()
			//	<< "twaitsize" << twaitsize;
		}
		Q_ASSERT(twaitsize >= 0);
		if (strm)
			strm->bytesWritten(pkt.payloadSize());
			// XXX delay and coalesce signal

		// fall through...

	case AttachPacket:
		if (tcuratt && tcuratt->flow == flow && !tcuratt->isAcked()) {
			// We've gotten our first ack for a new attachment.
			// Save the rxseq the ack came in on
			// as the attachment's reference pktseq.
			qDebug() << this << "got attach ack" << rxseq;
			tcuratt->setActive(rxseq);
			init = false;

			// Normal data transmission may now proceed.
			txenqflow();

			// Notify anyone interested that we're attached.
			attached();
			if (strm && state == Connected)
				strm->linkUp();
		}
		break;

	case AckPacket:
		// nothing to do
		break;

	case DatagramPacket:
		tflt -= pkt.payloadSize();	// no longer "in flight"
		Q_ASSERT(tflt >= 0);
		break;

	// XXX case DetachPacket:
	// XXX case ResetPacket:
	default:
		qDebug() << this << "got ack for unknown packet" << pkt.type;
		break;
	}
}

bool BaseStream::missed(StreamFlow *, const Packet &pkt)
{
	Q_ASSERT(pkt.late);

	//qDebug() << this << "missed bsn" << pkt.tsn
	//	<< "size" << pkt.buf.size();

	switch (pkt.type) {
	case DataPacket:
		qDebug() << this << "retransmit bsn" << pkt.tsn
			<< "size" << pkt.buf.size() - hdrlenData;

		// Mark the segment no longer "in flight".
		endflight(pkt);

		txenqueue(pkt);	// Retransmit reliable segments...
		return true;	// ...but keep the tx record until expiry
				// in case it gets acked late!
	case AttachPacket:
		qDebug() << "Attach packet lost: trying again to attach";
		txenqflow();
		return true;

	case DatagramPacket:
		qDebug() << "Datagram packet lost: oops, gone for good";

		// Mark the segment no longer "in flight".
		// We know we'll only do this once per DatagramPacket
		// because we drop it immediately below ("return false");
		// thus acked() cannot be called on it after this.
		tflt -= pkt.payloadSize();	// no longer "in flight"
		Q_ASSERT(tflt >= 0);

		return false;

	default:
		qDebug() << this << "missed unknown packet" << pkt.type;
		return false;
	}
}

void BaseStream::expire(StreamFlow *, const Packet &)
{
	// do nothing for now
}

void BaseStream::endflight(const Packet &pkt)
{
	// Cancel its allocation in our or our parent stream's state,
	// according to the type of packet actually sent.
	DataHeader *hdr = (DataHeader*)(pkt.buf.data() + Flow::hdrlen);
	if ((hdr->type >> typeShift) == InitPacket) {
		if (parent) {
			parent->tflt -= pkt.payloadSize();
			//qDebug() << this << "endflight" << pkt.tsn
			//	<< "bytes in flight (parent)"
			//	<< parent->tflt;
			Q_ASSERT(parent->tflt >= 0);
		}
	} else {	// Reply or Data packet
		tflt -= pkt.payloadSize();
		//qDebug() << this << "endflight" << pkt.tsn
		//	<< "bytes in flight" << tflt;
	}
	// XXX Q_ASSERT(tflt >= 0);
}

// Main packet receive entrypoint, called from StreamFlow::flowReceive()
bool BaseStream::receive(qint64 pktseq, QByteArray &pkt, StreamFlow *flow)
{
	if (pkt.size() < hdrlenMin) {
		qDebug("BaseStream::receive: got runt packet");
		return false;	// XX Protocol error: close flow?
	}
	StreamHeader *hdr = (StreamHeader*)(pkt.data() + Flow::hdrlen);

	// Handle the packet according to its type
	switch ((PacketType)(hdr->type >> typeShift)) {
	case InitPacket:	return rxInitPacket(pktseq, pkt, flow);
	case ReplyPacket:	return rxReplyPacket(pktseq, pkt, flow);
	case DataPacket:	return rxDataPacket(pktseq, pkt, flow);
	case DatagramPacket:	return rxDatagramPacket(pktseq, pkt, flow);
	case AckPacket:		return rxAckPacket(pktseq, pkt, flow);
	case ResetPacket:	return rxResetPacket(pktseq, pkt, flow);
	case AttachPacket:	return rxAttachPacket(pktseq, pkt, flow);
	case DetachPacket:	return rxDetachPacket(pktseq, pkt, flow);
	default:
		qDebug("BaseStream::receive: unknown packet type %x",
			hdr->type);
		return false;	// XX Protocol error: close flow?
	};
}

bool BaseStream::rxInitPacket(quint64 pktseq, QByteArray &pkt, StreamFlow *flow)
{
	if (pkt.size() < hdrlenInit) {
		qDebug("BaseStream::receive: got runt packet");
		return false;	// XX Protocol error: close flow?
	}
	InitHeader *hdr = (InitHeader*)(pkt.data() + Flow::hdrlen);

	// Look up the stream - if it already exists,
	// just dispatch it directly as if it were a data packet.
	StreamId sid = ntohs(hdr->sid);
	Attachment *att = flow->rxsids.value(sid);
	if (att != NULL) {
		if (pktseq < att->sidseq)
			att->sidseq = pktseq;	// earlier Init; that's OK.
		flow->acksid = sid;
		// XXX only calcTransmitWindow if rx'd in-order!?
		att->strm->calcTransmitWindow(hdr->win);
		att->strm->rxData(pkt, ntohs(hdr->tsn));
		return true;	// Acknowledge the packet
	}

	// Doesn't yet exist - look up the parent stream.
	quint16 psid = ntohs(hdr->rsid);
	Attachment *patt = flow->rxsids.value(psid);
	if (patt == NULL) {
		// The parent SID is in error, so reset that SID.
		// Ack the pktseq first so peer won't ignore the reset!
		qDebug("rxInitPacket: unknown parent stream ID");
		flow->acknowledge(pktseq, false);
		txReset(flow, psid, resetDirFlag);
		return false;
	}
	if (pktseq < patt->sidseq) {
		qDebug() << "rxInitPacket: stale wrt parent sidseq";
		return false;	// silently drop stale packet
	}
	BaseStream *pbs = patt->strm;

	// Extrapolate the sender's stream counter from the new SID it sent,
	// and use it to form the new stream's USID.
	StreamCtr ctr = flow->rxctr + (qint16)(sid - (qint16)flow->rxctr);
	UniqueStreamId usid(ctr, flow->rxChannelId());

	// Create the new substream.
	BaseStream *nbs = pbs->rxSubstream(pktseq, flow, sid, 0, usid);
	if (nbs == NULL)
		return false;	// substream creation failed

	// Now process any data segment contained in this Init packet.
	flow->acksid = sid;
	// XXX only calcTransmitWindow if rx'd in-order!?
	nbs->calcTransmitWindow(hdr->win);
	nbs->rxData(pkt, ntohs(hdr->tsn));

	return false;	// Already acknowledged in rxSubstream().
}

BaseStream *BaseStream::rxSubstream(
		quint64 pktseq, StreamFlow *flow,
		quint16 sid, unsigned slot, const UniqueStreamId &usid)
{
	// Make sure we're allowed to create a child stream.
	if (!isListening()) {
		// The parent SID is not in error, so just reset the new child.
		// Ack the pktseq first so peer won't ignore the reset!
		qDebug("rxInitPacket: other side trying to create substream, "
			"but we're not listening.");
		flow->acknowledge(pktseq, false);
		txReset(flow, sid, resetDirFlag);
		return NULL;
	}

	// Mark the Init packet received now, before transmitting the Reply;
	// otherwise the Reply won't acknowledge the Init
	// and the peer will reject it as a stale packet.
	flow->acknowledge(pktseq, true);

	// Create the child stream.
	BaseStream *nbs = new BaseStream(flow->host(), peerid, this);

	// We'll accept the new stream: this is the point of no return.
	qDebug() << nbs << "accepting stream" << usid;

	// Extrapolate the sender's stream counter from the new SID it sent.
	StreamCtr ctr = flow->rxctr + (qint16)(sid - (qint16)flow->rxctr);
	if (ctr > flow->rxctr)
		flow->rxctr = ctr;

	// Use this stream counter to form the new stream's USID.
	nbs->setUsid(usid);

	// Automatically attach the child via its appropriate receive-slot.
	nbs->ratt[slot].setActive(flow, sid, pktseq);

	// If this is a new top-level application stream,
	// we expect a service request before application data.
	if (this == &flow->root) {
		nbs->state = Accepting;	// Service request expected
	} else {
		nbs->state = Connected;
		this->rsubs.enqueue(nbs);
		connect(nbs, SIGNAL(readyReadMessage()),
			this, SLOT(subReadMessage()));
		if (this->strm)
			this->strm->newSubstream();
	}

	return nbs;
}

bool BaseStream::rxReplyPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow)
{
	if (pkt.size() < hdrlenReply) {
		qDebug("BaseStream::receive: got runt packet");
		return false;	// XX Protocol error: close flow?
	}
	ReplyHeader *hdr = (ReplyHeader*)(pkt.data() + Flow::hdrlen);

	// Look up the stream - if it already exists,
	// just dispatch it directly as if it were a data packet.
	StreamId sid = ntohs(hdr->sid);
	Attachment *att = flow->rxsids.value(sid);
	if (att != NULL) {
		if (pktseq < att->sidseq)
			att->sidseq = pktseq;	// earlier Reply; that's OK.
		flow->acksid = sid;
		// XXX only calcTransmitWindow if rx'd in-order!?
		att->strm->calcTransmitWindow(hdr->win);
		att->strm->rxData(pkt, ntohs(hdr->tsn));
		return true;	// Acknowledge the packet
	}

	// Doesn't yet exist - look up the reference stream in our SID space.
	quint16 rsid = ntohs(hdr->rsid);
	Attachment *ratt = flow->txsids.value(rsid);
	if (ratt == NULL) {
		// The reference SID supposedly in our own space is invalid!
		// Respond with a reset indicating that SID in our space.
		// Ack the pktseq first so peer won't ignore the reset!
		qDebug("rxReplyPacket: unknown reference stream ID");
		flow->acknowledge(pktseq, false);
		txReset(flow, rsid, 0);
		return false;
	}
	if (pktseq < ratt->sidseq) {
		qDebug() << "rxReplyPacket: stale - pktseq" << pktseq
			<< "sidseq" << ratt->sidseq;
		return false;	// silently drop stale packet
	}
	BaseStream *bs = ratt->strm;

	qDebug() << bs << "accepting reply" << bs->usid;

	// OK, we have the stream - just create the receive-side attachment.
	bs->ratt[0].setActive(flow, sid, pktseq);

	// Now process any data segment contained in this Init packet.
	flow->acksid = sid;
	// XXX only calcTransmitWindow if rx'd in-order!?
	bs->calcTransmitWindow(hdr->win);
	bs->rxData(pkt, ntohs(hdr->tsn));

	return true;	// Acknowledge the packet
}

bool BaseStream::rxDataPacket(quint64 pktseq, QByteArray &pkt, StreamFlow *flow)
{
	if (pkt.size() < hdrlenData) {
		qDebug("BaseStream::receive: got runt packet");
		return false;	// XX Protocol error: close flow?
	}
	DataHeader *hdr = (DataHeader*)(pkt.data() + Flow::hdrlen);

	// Look up the stream the data packet belongs to.
	quint16 sid = ntohs(hdr->sid);
	Attachment *att = flow->rxsids.value(sid);
	if (att == NULL) {
		// Respond with a reset for the unknown stream ID.
		// Ack the pktseq first so peer won't ignore the reset!
		qDebug("rxDataPacket: unknown stream ID");
		flow->acknowledge(pktseq, false);
		txReset(flow, sid, resetDirFlag);
		return false;
	}
	if (pktseq < att->sidseq) {
		qDebug() << "rxDataPacket: stale wrt sidseq";
		return false;	// silently drop stale packet
	}

	// Process the data, using the full 32-bit TSN.
	flow->acksid = sid;
	// XXX only calcTransmitWindow if rx'd in-order!?
	att->strm->calcTransmitWindow(hdr->win);
	att->strm->rxData(pkt, ntohl(hdr->tsn));

	return true;	// Acknowledge the packet
}

void BaseStream::rxData(QByteArray &pkt, quint32 byteseq)
{
	//qDebug() << this << "rxData" << byteseq
	//	<< (pkt.size() - hdrlenData);

	RxSegment rseg;
	rseg.rsn = byteseq;
	rseg.buf = pkt;
	rseg.hdrlen = hdrlenData;

	if (endread) {
		// Ignore anything we receive past end of stream
		// (which we may have forced from our end via close()).
		qDebug() << "Ignoring segment received after end-of-stream";
		Q_ASSERT(rahead.isEmpty());
		Q_ASSERT(rsegs.isEmpty());
		return;
	}

	int segsize = rseg.segmentSize();
	//qDebug() << this << "received" << rseg.rsn << "size" << segsize
	//	<< "flags" << rseg.flags() << "stream-rsn" << rsn;

#if 0
	// See if the sender wasn't supposed to send this segment yet.
	// For now, whine but accept it anyway... (XX)
	qint32 rsnhi = byteseq + segsize;
	if (rsnhi > rwinhi)
		qDebug() << this << "received segment ending at" << rsnhi
			<< "but receive window ends at" << rwinhi;
#endif

	// See where this packet fits in
	int rsndiff = rseg.rsn - rsn;
	if (rsndiff <= 0) {

		// The segment is at or before our current receive position.
		// How much of its data, if any, is actually useful?
		// Note that we must process packets at our RSN with no data,
		// because they might have important flags.
		int actsize = segsize + rsndiff;
		if (actsize < 0 || (actsize == 0 && !rseg.hasFlags())) {
			// The packet is way out of date -
			// its end doesn't even come up to our current RSN.
			qDebug() << "Duplicate segment at RSN" << rseg.rsn
				<< "size" << segsize;
			goto done;
		}
		rseg.hdrlen -= rsndiff;	// Merge useless data into "headers"
		//qDebug() << "actsize" << actsize << "flags" << rseg.flags();

		// It gives us exactly the data we want next - quelle bonheur!
		bool wasempty = !hasBytesAvailable();
		bool wasnomsgs = !hasPendingMessages();
		bool closed = false;
		rsegs.enqueue(rseg);
		rsn += actsize;
		ravail += actsize;
		rmsgavail += actsize;
		rbufused += actsize;
		if ((rseg.flags() & (dataMessageFlag | dataCloseFlag))
				&& (rmsgavail > 0)) {
			//qDebug() << this << "received message";
			rmsgsize.enqueue(rmsgavail);
			rmsgavail = 0;
		}
		if (rseg.flags() & dataCloseFlag)
			closed = true;

		// Then pull anything we can from the reorder buffer
		for (; !rahead.isEmpty(); rahead.removeFirst()) {
			RxSegment &rseg = rahead.first();
			int segsize = rseg.segmentSize();

			int rsndiff = rseg.rsn - rsn;
			if (rsndiff > 0)
				break;	// There's still a gap

			// Account for removal of this segment from rahead;
			// below we'll re-add whatever part of it we use.
			rbufused -= segsize;

			//qDebug() << "Pull segment at" << rseg.rsn
			//	<< "of size" << segsize
			//	<< "from reorder buffer";

			int actsize = segsize + rsndiff;
			if (actsize < 0 || (actsize == 0 && !rseg.hasFlags()))
				continue;	// No useful data: drop
			rseg.hdrlen -= rsndiff;

			// Consume this segment too.
			rsegs.enqueue(rseg);
			rsn += actsize;
			ravail += actsize;
			rmsgavail += actsize;
			rbufused += actsize;
			if ((rseg.flags() & (dataMessageFlag | dataCloseFlag))
					&& (rmsgavail > 0)) {
				rmsgsize.enqueue(rmsgavail);
				rmsgavail = 0;
			}
			if (rseg.flags() & dataCloseFlag)
				closed = true;
		}

		// If we're at the end of stream with no data to read,
		// go into the end-of-stream state immediately.
		// We must do this because readData() may never
		// see our queued zero-length segment if ravail == 0.
		if (closed && ravail == 0) {
			shutdown(Stream::Read);
			readyReadMessage();
			if (isLinkUp() && strm) {
				strm->readyRead();
				strm->readyReadMessage();
			}
			goto done;
		}

		// Notify the client if appropriate
		if (wasempty) {
			if (state == Connected && strm)
				strm->readyRead();
		}
		if (wasnomsgs && hasPendingMessages()) {
			if (state == Connected) {
				readyReadMessage();
				if (strm)
					strm->readyReadMessage();
			} else if (state == WaitService) {
				gotServiceReply();
			} else if (state == Accepting)
				gotServiceRequest();
		}

	} else if (rsndiff > 0) {

		// It's out of order beyond our current receive sequence -
		// stash it in a re-order buffer, sorted by rsn.
		//qDebug() << "Received out-of-order segment at" << rseg.rsn
		//	<< "size" << segsize;
		int lo = 0, hi = rahead.size();
		if (hi > 0 && (rahead[hi-1].rsn - rsn) < rsndiff) {
			// Common case: belongs at end of rahead list.
			rahead.append(rseg);
			rbufused += segsize;
			goto done;
		}

		// Binary search for the correct position.
		while (lo < hi) {
			int mid = (hi + lo) / 2;
			if ((rahead[mid].rsn - rsn) < rsndiff)
				lo = mid+1;
			else
				hi = mid;
		}

		// Don't save duplicate segments
		// (unless the duplicate actually has more data or new flags).
		if (lo < rahead.size() && rahead[lo].rsn == rseg.rsn
				&& segsize <= rahead[lo].segmentSize()
				&& rseg.flags() == rahead[lo].flags()) {
			qDebug("rxseg duplicate out-of-order segment - RSN %d",
				rseg.rsn);
			goto done;
		}

		rbufused += segsize;
		rahead.insert(lo, rseg);
	}
	done:

	// Recalculate the receive window now that we've probably
	// consumed some buffer space.
	calcReceiveWindow();
}

bool BaseStream::rxDatagramPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow)
{
	if (pkt.size() < hdrlenDatagram) {
		qDebug("BaseStream::receive: got runt packet");
		return false;	// XX Protocol error: close flow?
	}
	DatagramHeader *hdr = (DatagramHeader*)(pkt.data() + Flow::hdrlen);

	// Look up the stream for which the datagram is a substream.
	quint16 sid = ntohs(hdr->sid);
	Attachment *att = flow->rxsids.value(sid);
	if (att == NULL) {
		// Respond with a reset for the unknown stream ID.
		// Ack the pktseq first so peer won't ignore the reset!
		resetsid:
		qDebug("rxDatagramPacket: unknown stream ID");
		flow->acknowledge(pktseq, false);
		txReset(flow, sid, resetDirFlag);
		return false;
	}
	flow->acksid = sid;
	if (pktseq < att->sidseq)
		return false;	// silently drop stale packet
	BaseStream *bs = att->strm;

	// XXX only calcTransmitWindow if rx'd in-order!?
	//bs->calcTransmitWindow(hdr->win);	XXX ???

	if (bs->state != Connected)
		goto resetsid;	// Only accept datagrams while connected

	int flags = hdr->type;
	//qDebug() << "rxDatagramSegment" << segsize << "type" << type;

	if (!(flags & dgramBeginFlag) || !(flags & dgramEndFlag)) {
		qWarning("OOPS, don't yet know how to reassemble datagrams");
		return false;	// XXX
	}

	// Build a pseudo-Stream object encapsulating the datagram.
	DatagramStream *dg = new DatagramStream(bs->h, pkt, hdrlenDatagram);
	bs->rsubs.enqueue(dg);
	// Don't need to connect to the sub's readyReadMessage() signal
	// because we already know the sub is completely received...
	if (bs->strm) {
		bs->strm->newSubstream();
		bs->strm->readyReadDatagram();
	}

	return true;	// Acknowledge the packet
}

bool BaseStream::rxAckPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow)
{
	//qDebug() << "rxAckPacket" << this;

	// Count this explicit ack packet as received,
	// but do NOT send another ack just to ack this ack!
	flow->acknowledge(pktseq, false);

	// Decode the packet header
	StreamHeader *hdr = (StreamHeader*)(pkt.data() + Flow::hdrlen);

	// Look up the stream the data packet belongs to.
	// The SID field in an Ack packet is in our transmit SID space,
	// because it reflects data our peer is receiving.
	quint16 sid = ntohs(hdr->sid);
	Attachment *att = flow->txsids.value(sid);
	if (att == NULL) {
		// Respond with a reset for the unknown stream ID.
		// Ack the pktseq first so peer won't ignore the reset!
		qDebug("rxAckPacket: unknown stream ID");
#if 0	// XXX do we want to do this, or not ???
		txReset(flow, sid, resetDirFlag);
#endif
		return false;
	}
	if (pktseq < att->sidseq) {
		qDebug() << "rxDataPacket: stale wrt sidseq";
		return false;	// silently drop stale packet
	}

	// Process the receive window update.
	// XXX only calcTransmitWindow if rx'd in-order!?
	att->strm->calcTransmitWindow(hdr->win);

	return false;	// Already accepted the packet above.
}

bool BaseStream::rxResetPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow)
{
	Q_ASSERT(0);	// XXX
	(void)pktseq; (void)pkt; (void)flow;
	return false;
}

bool BaseStream::rxAttachPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow)
{
	qDebug() << "rxAttachPacket size" << pkt.size();
	if (pkt.size() < hdrlenDatagram) {
		qDebug("BaseStream::rxAttachPacket: got runt packet");
		return false;	// XX Protocol error: close flow?
	}

	// Decode the packet header
	AttachHeader *hdr = (AttachHeader*)(pkt.data() + Flow::hdrlen);
	quint16 sid = ntohs(hdr->sid);
	unsigned init = hdr->type & attachInitFlag;
	unsigned slot = hdr->type & attachSlotMask;
	Q_ASSERT(attachSlotMask == maxAttach-1);

	// Decode the USID(s) in the body
	XdrStream xs(&pkt, QIODevice::ReadOnly);
	xs.skipRawData(hdrlenAttach);
	UniqueStreamId usid, pusid;
	xs >> usid;
	if (init)
		xs >> pusid;
	if (xs.status() != xs.Ok || usid.isNull() || (init && pusid.isNull())) {
		qDebug("BaseStream::rxAttachPacket: invalid attach packet");
		return false;	// XX protocol error - close flow?
	}

	// First try to look up the stream by its own USID.
	StreamPeer *peer = flow->peer;
	BaseStream *bs = peer->usids.value(usid);
	if (bs != NULL) {
		// Found it: the stream already exists, just attach it.
		flow->acksid = sid;
		RxAttachment &rslot = bs->ratt[slot];
		if (rslot.isActive()) {
			if (rslot.flow == flow &&
					rslot.sid == sid) {
				qDebug() << bs << "redundant attach"
					<< bs->usid;
				rslot.sidseq = qMin(rslot.sidseq, pktseq);
				return true;
			}
			qDebug() << bs << "replacing attach slot" << slot;
			rslot.clear();
		}
		qDebug() << bs << "accepting attach" << bs->usid;
		rslot.setActive(flow, sid, pktseq);
		return true;
	}

	// Stream doesn't exist - lookup parent if it's an init-attach.
	BaseStream *pbs = NULL;
	if (init)
		pbs = peer->usids.value(pusid);
	if (pbs != NULL) {
		// Found it: create and attach a child stream.
		flow->acksid = sid;
		bs = pbs->rxSubstream(pktseq, flow, sid, slot, usid);
		if (bs == NULL)
			return false;	// Substream creation failed
		return false;	// Already acked in rxSubstream()
	}

	// No way to attach the stream - just reset it.
	qDebug() << "rxAttachPacket: unknown stream" << usid;
	flow->acknowledge(pktseq, false);
	txReset(flow, sid, resetDirFlag);
	return false;
}

bool BaseStream::rxDetachPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow)
{
	(void)pktseq; (void)pkt; (void)flow;
	Q_ASSERT(0);	// XXX
	return false;
}

void BaseStream::calcReceiveWindow()
{
	Q_ASSERT(rcvbuf > 0);

	// Calculate the current receive window size
	qint32 rwin = qMax(0, rcvbuf - rbufused);

	// If all of our buffer usage consists of out-of-order packets,
	// ensure that the sender can make progress toward filling the gaps.
	// This should only ever be an issue
	// if we shrink the receive window abruptly,
	// leaving gaps in a formerly-large window.
	// (It's best just to avoid shrinking the receive window abruptly.)
	if (ravail == 0 && rbufused > 0)
		rwin = qMax(rwin, minReceiveBuffer);

	// Calculate the conservative receive window exponent
	int i = 0;
	while (((2 << i) - 1) <= rwin)
		i++;
	rwinbyte = i;	// XX control bits?

	//qDebug() << this << "buffered" << ravail << "+" << (rbufused - ravail)
	//	<< "rwin" << rwin << "exp" << i;
}

void BaseStream::calcTransmitWindow(quint8 win)
{
	qint32 oldtwin = twin;

	// Calculate the new transmit window
	int i = win & 0x1f;
	twin = (1 << i) - 1;

	//qDebug() << this << "transmit window" << oldtwin << "->" << twin
	//	<< "in use" << tflt;

	if (twin > oldtwin)
		txenqflow(true);
}

int BaseStream::readData(char *data, int maxSize)
{
	int actSize = 0;
	while (maxSize > 0 && ravail > 0) {
		Q_ASSERT(!endread);
		Q_ASSERT(!rsegs.isEmpty());
		RxSegment rseg = rsegs.dequeue();

		int size = rseg.segmentSize();
		Q_ASSERT(size >= 0);

		// XXX BUG: this breaks if we try to read a partial segment!
		Q_ASSERT(maxSize >= size);

		// Copy the data (or just drop it if data == NULL).
		if (data != NULL) {
			memcpy(data, rseg.buf.data() + rseg.hdrlen, size);
			data += size;
		}
		actSize += size;
		maxSize -= size;

		// Adjust the receive stats
		ravail -= size;
		rbufused -= size;
		Q_ASSERT(ravail >= 0);
		if (hasPendingMessages()) {

			// We're reading data from a queued message.
			qint64 &headsize = rmsgsize.head();
			headsize -= size;
			Q_ASSERT(headsize >= 0);

			// Always stop at the next message boundary.
			if (headsize == 0) {
				rmsgsize.removeFirst();
				break;
			}
		} else {

			// No queued messages - just read raw data.
			rmsgavail -= size;
			Q_ASSERT(rmsgavail >= 0);
		}

		// If this segment has the end-marker set, that's it...
		if (rseg.flags() & dataCloseFlag)
			shutdown(Stream::Read);
	}

	// Recalculate the receive window,
	// now that we've (presumably) freed some buffer space.
	calcReceiveWindow();

	return actSize;
}

int BaseStream::readMessage(char *data, int maxSize)
{
	if (!hasPendingMessages())
		return -1;	// No complete messages available
	// XXX don't deadlock if a way-too-large message comes in...

	// Read as much of the next queued message as we have room for
	int oldrmsgs = rmsgsize.size();
	int actsize = BaseStream::readData(data, maxSize);
	Q_ASSERT(actsize > 0);

	// If the message is longer than the supplied buffer, drop the rest.
	if (rmsgsize.size() == oldrmsgs) {
		int skipsize = BaseStream::readData(NULL, 1 << 30);
		Q_ASSERT(skipsize > 0);
	}
	Q_ASSERT(rmsgsize.size() == oldrmsgs - 1);

	return actsize;
}

QByteArray BaseStream::readMessage(int maxSize)
{
	int msgsize = pendingMessageSize(); 
	if (msgsize <= 0)
		return QByteArray();	// No complete messages available

	// Read the next message into a new QByteArray
	QByteArray buf;
	int bufsize = qMin(msgsize, maxSize);
	buf.resize(bufsize);
	int actsize = readMessage(buf.data(), bufsize);
	Q_ASSERT(actsize == bufsize);
	return buf;
}

int BaseStream::writeData(const char *data, int totsize, quint8 endflags)
{
	Q_ASSERT(!endwrite);
	qint64 actsize = 0;
	do {
		// Choose the size of this segment.
		int size = mtu;
		quint8 flags = 0;
		if (totsize <= size) {
			flags = dataPushFlag | endflags;
			size = totsize;
		}
		//qDebug() << "Transmit segment at" << tasn << "size" << size;

		// Build the appropriate packet header.
		Packet p(this, DataPacket);
		p.tsn = tasn;
		p.buf.resize(hdrlenData + size);

		// Prepare the header
		DataHeader *hdr = (DataHeader*)(p.buf.data() + Flow::hdrlen);
		// hdr->sid - later
		hdr->type = flags;	// Major type filled in later
		// hdr->win - later
		// hdr->tsn - later
		p.hdrlen = hdrlenData;

		// Advance the TSN to account for this data.
		tasn += size;

		// Copy in the application payload
		char *payload = (char*)(hdr+1);
		memcpy(payload, data, size);

		// Hold onto the packet data until it gets ACKed
		twait.insert(p.tsn);
		twaitsize += size;
		//qDebug() << "twait insert" << p.tsn << "size" << size
		//	<< "new cnt" << twait.size()
		//	<< "twaitsize" << twaitsize;

		// Queue up the segment for transmission ASAP
		txenqueue(p);

		// On to the next segment...
		data += size;
		totsize -= size;
		actsize += size;
	} while (totsize > 0);

	if (endflags & dataCloseFlag)
		endwrite = true;

	return actsize;
}

qint32 BaseStream::writeDatagram(const char *data, qint32 totsize,
				bool reliable)
{
	if (reliable || totsize > mtu /* XXX maxStatelessDatagram */ )
	{
		// Datagram too large to send using the stateless optimization:
		// just send it as a regular substream.
		qDebug() << this << "sending large datagram, size" << totsize;
		AbstractStream *sub = openSubstream();
		if (sub == NULL)
			return -1;
		sub->writeData(data, totsize, dataCloseFlag);
		// sub will self-destruct when sent and acked.
		return totsize;
	}

	qint32 remain = totsize;
	quint8 flags = dgramBeginFlag;
	do {
		// Choose the size of this fragment.
		int size = mtu;
		if (remain <= size) {
			flags |= dgramEndFlag;
			size = remain;
		}

		// Build the appropriate packet header.
		Packet p(this, DatagramPacket);
		char *payload;

		// Assign this packet an ASN as if it were a data segment,
		// but don't actually allocate any TSN bytes to it -
		// this positions the datagram in FIFO order in tqueue.
		// XX is this necessarily what we actually want?
		p.tsn = tasn;

		// Build the DatagramHeader.
		p.buf.resize(hdrlenDatagram + size);
		Q_ASSERT(hdrlenDatagram == Flow::hdrlen
					+ sizeof(DatagramHeader));
		DatagramHeader *hdr = (DatagramHeader*)
					(p.buf.data() + Flow::hdrlen);
		// hdr->sid - later
		hdr->type = (DatagramPacket << typeShift) | flags;
		// hdr->win - later

		p.hdrlen = hdrlenDatagram;
		payload = (char*)(hdr+1);

		// Copy in the application payload
		memcpy(payload, data, size);

		// Queue up the packet
		txenqueue(p);

		// On to the next fragment...
		data += size;
		remain -= size;
		flags &= ~dgramBeginFlag;

	} while (remain > 0);
	Q_ASSERT(flags & dgramEndFlag);

	// Once we've enqueued all the fragments of the datagram,
	// add our stream to our flow's transmit queue,
	// and start transmitting immediately if possible.
	txenqflow(true);

	return totsize;
}

AbstractStream *BaseStream::acceptSubstream()
{
	if (rsubs.isEmpty())
		return NULL;

	AbstractStream *sub = rsubs.dequeue();
	QObject::disconnect(sub, SIGNAL(readyReadMessage()),
				this, SLOT(subReadMessage()));
	return sub;
}

AbstractStream *BaseStream::getDatagram()
{
	// Scan through the list of queued substreams
	// for one with a complete record waiting to be read.
	for (int i = 0; i < rsubs.size(); i++) {
		AbstractStream *sub = rsubs[i];
		if (!sub->hasPendingMessages())
			continue;
		rsubs.removeAt(i);
		return sub;
	}

	setError(tr("No datagrams available for reading"));
	return NULL;
}

int BaseStream::readDatagram(char *data, int maxSize)
{
	AbstractStream *sub = getDatagram();
	if (!sub)
		return -1;

	int act = sub->readData(data, maxSize);
	sub->shutdown(Stream::Reset);	// sub will self-destruct
	return act;
}

QByteArray BaseStream::readDatagram(int maxSize)
{
	AbstractStream *sub = getDatagram();
	if (!sub)
		return QByteArray();

	QByteArray data = sub->readMessage(maxSize);
	sub->shutdown(Stream::Reset);	// sub will self-destruct
	return data;
}

void BaseStream::subReadMessage()
{
	// When one of our queued subs emits a readyReadMessage() signal,
	// we have to forward that via our readyReadDatagram() signal.
	if (strm)
		strm->readyReadDatagram();
}

void BaseStream::setReceiveBuffer(int size)
{
	if (size < minReceiveBuffer) {
		qWarning("Receive buffer size %d too small, using %d",
			size, minReceiveBuffer);
		size = minReceiveBuffer;
	}
	rcvbuf = size;
}

void BaseStream::setChildReceiveBuffer(int size)
{
	if (size < minReceiveBuffer) {
		qWarning("Child receive buffer size %d too small, using %d",
			size, minReceiveBuffer);
		size = minReceiveBuffer;
	}
	crcvbuf = size;
}

void BaseStream::shutdown(Stream::ShutdownMode mode)
{
	// XXX self-destruct when done, if appropriate

	if (mode & Stream::Reset)
		return disconnect();	// No graceful close necessary

	if (isLinkUp() && !endread && (mode & Stream::Read)) {
		// Shutdown for reading
		ravail = 0;
		rmsgavail = 0;
		rbufused = 0;
		rahead.clear();
		rsegs.clear();
		rmsgsize.clear();
		endread = true;
	}

	if (isLinkUp() && !endwrite && (mode & Stream::Write)) {
		// Shutdown for writing
		writeData(NULL, 0, dataCloseFlag);
	}
}

void BaseStream::fail(const QString &err)
{
	disconnect();
	setError(err);
}

void BaseStream::disconnect()
{
	//qDebug() << "Stream" << this << "disconnected: state" << state;

	// XXX disconnect from stream
	//Q_ASSERT(0);

	state = Disconnected;
	if (strm) {
		strm->linkDown();
		// XXX strm->reset?
	}
}

#ifndef QT_NO_DEBUG
void BaseStream::dump()
{
	qDebug() << "Stream" << this << "state" << state;
	qDebug() << "  TSN" << tasn << "tqueue" << tqueue.size();
	qDebug() << "  RSN" << rsn << "ravail" << ravail
		<< "rahead" << rahead.size() << "rsegs" << rsegs.size()
		<< "rmsgavail" << rmsgavail << "rmsgs" << rmsgsize.size();
}
#endif


