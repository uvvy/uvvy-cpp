
#include <netinet/in.h>

#include <QtDebug>

#include "strm/base.h"
#include "strm/peer.h"
#include "strm/sflow.h"

using namespace SST;


////////// StreamFlow //////////

StreamFlow::StreamFlow(Host *h, StreamPeer *peer, const QByteArray &peerid)
:	Flow(h, peer),
	peer(peer),
	root(h, peerid, NULL),
	txctr(1),
	txackctr(0),
	rxctr(0)
{
	root.setParent(NULL);	// XXX
	root.state = BaseStream::Connected;

	// Pre-attach the root stream to the flow in both directions
	root.tatt[0].setAttaching(this, sidRoot);
	root.tatt[0].setActive(1);
	root.tcuratt = &root.tatt[0];

	root.ratt[0].setActive(this, sidRoot, 1);

	// Listen on the root stream for top-level application streams
	root.listen(Stream::Unlimited);

	//XXX channel IDs

	connect(this, SIGNAL(readyTransmit()),
		this, SLOT(gotReadyTransmit()));
	connect(this, SIGNAL(linkStatusChanged(LinkStatus)),
		this, SLOT(gotLinkStatusChanged(LinkStatus)));
}

StreamFlow::~StreamFlow()
{
	//qDebug() << this << "~StreamFlow to" << remoteEndpoint().toString()
	//	<< "lchan" << localChannel() << "rchan" << remoteChannel();

	disconnect(this, SIGNAL(linkStatusChanged(LinkStatus)),
		this, SLOT(gotLinkStatusChanged(LinkStatus)));

	stop();

	root.state = BaseStream::Disconnected;
}

void StreamFlow::enqueueStream(BaseStream *strm)
{
	// Find the correct position at which to enqueue this stream,
	// based on priority.
	int i = 0;
	while (i < tstreams.size()
			&& tstreams[i]->priority() >= strm->priority())
		i++;

	// Insert.
	tstreams.insert(i, strm);
}

void StreamFlow::detachAll()
{
	// Save off and clear the flow's entire ackwait table -
	// it'll be more efficient to go through it once
	// and send all the waiting packets back to their streams,
	// than for each stream to pull out its packets individually.
	QHash<qint64,BaseStream::Packet> ackbak = ackwait;
	ackwait.clear();

	// Detach all the streams with transmit-attachments to this flow.
	foreach (TxAttachment *att, txsids)
		att->clear();
	Q_ASSERT(txsids.isEmpty());

	// Finally, send back all the waiting packets to their streams.
	qDebug() << this << "returning" << ackbak.size() << "packets";
	foreach (BaseStream::Packet p, ackbak) {
		Q_ASSERT(!p.isNull());
		Q_ASSERT(p.strm);
		if (!p.late) {
			p.late = true;
			p.strm->missed(this, p);
		} else
			p.strm->expire(this, p);
	}
}

bool StreamFlow::transmitAck(QByteArray &pkt, quint64 ackseq, unsigned ackct)
{
	// Pick a stream on which to send a window update -
	// for now, just the most recent stream on which we received a segment.
	RxAttachment *ratt = rxsids.value(acksid);
	if (ratt == NULL)
		ratt = rxsids.value(sidRoot);

	// Build a bare Ack packet header
	if (pkt.size() < hdrlenAck)
		pkt.resize(hdrlenAck);
	AckHeader *hdr = (AckHeader*)(pkt.data() + Flow::hdrlen);
	hdr->sid = htons(ratt->sid);
	hdr->type = AckPacket << typeShift;
	hdr->win = ratt->strm->receiveWindow();

	// Let flow protocol put together its part of the packet and send it.
	return Flow::transmitAck(pkt, ackseq, ackct);
}

void StreamFlow::gotReadyTransmit()
{
	if (tstreams.isEmpty())
		return;

	// Round-robin between our streams for now.
	do {
		// Grab the next stream in line to transmit
		BaseStream *strm = tstreams.dequeue();

		// Allow it to transmit one packet.
		// It will add itself back onto tstreams if it has more.
		strm->transmit(this);

	} while (!tstreams.isEmpty() && mayTransmit());
}

void StreamFlow::acked(quint64 txseq, int npackets, quint64 rxseq)
{
	for (; npackets > 0; txseq++, npackets--) {
		// find and remove the packet
		BaseStream::Packet p = ackwait.take(txseq);
		if (p.isNull())
			continue;

		//qDebug() << "Got ack for packet" << txseq
		//	<< "of size" << p.buf.size();
		p.strm->acked(this, p, rxseq);
	}
}

void StreamFlow::missed(quint64 txseq, int npackets)
{
	for (; npackets > 0; txseq++, npackets--) {
		// find but don't remove (common case for missed packets)
		if (!ackwait.contains(txseq)) {
			//qDebug() << "Missed packet" << txseq
			//	<< "but can't find it!";
			continue;
		}
		BaseStream::Packet &p = ackwait[txseq];

		//qDebug() << "Missed packet" << txseq
		//	<< "of size" << p.buf.size();
		if (!p.late) {
			p.late = true;
			if (!p.strm->missed(this, p))
				ackwait.remove(txseq);
		}
	}
}

void StreamFlow::expire(quint64 txseq, int npackets)
{
	for (; npackets > 0; txseq++, npackets--) {
		// find and unconditionally remove packet when it expires
		BaseStream::Packet p = ackwait.take(txseq);
		if (p.isNull()) {
			//qDebug() << "Missed packet" << txseq
			//	<< "but can't find it!";
			continue;
		}

		//qDebug() << "Missed packet" << txseq
		//	<< "of size" << p.buf.size();
		p.strm->expire(this, p);
	}
}

bool StreamFlow::flowReceive(qint64 pktseq, QByteArray &pkt)
{
	return BaseStream::receive(pktseq, pkt, this);
}

void StreamFlow::gotLinkStatusChanged(LinkStatus newstatus)
{
	qDebug() << this << "gotLinkStatusChanged:" << newstatus;

	if (newstatus != LinkDown)
		return;

	// Link went down indefinitely - self-destruct.
	StreamPeer *peer = target();
	Q_ASSERT(peer);

	// If we were our target's primary flow, disconnect us.
	if (peer->flow == this) {
		qDebug() << "Primary flow to host ID" << peer->id.toBase64()
			<< "at endpoint" << remoteEndpoint().toString()
			<< "failed";
		peer->clearPrimary();
	}

	// Stop and destroy this flow.
	stop();
	deleteLater();
}

void StreamFlow::start(bool initiator)
{
	Flow::start(initiator);
	Q_ASSERT(isActive());

	// Set the root stream's USID based on our channel ID
	root.usid.chanId = initiator ? txChannelId() : rxChannelId();
	root.usid.streamCtr = 0;
	Q_ASSERT(!root.usid.chanId.isNull());

	// If our target doesn't yet have an active flow, use this one.
	// This way we either an incoming or outgoing flow can be a primary.
	target()->flowStarted(this);
}

void StreamFlow::stop()
{
	//qDebug() << "StreamFlow::stop";

	Flow::stop();

	// XXX clean up tstreams, ackwait

	// Detach and notify all affected streams.
	foreach (TxAttachment *att, txsids) {
		Q_ASSERT(att->flow == this);
		att->clear();
	}
	foreach (RxAttachment *att, rxsids) {
		Q_ASSERT(att->flow == this);
		att->clear();
	}
}


