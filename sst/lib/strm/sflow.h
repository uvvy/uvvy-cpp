#ifndef SST_STRM_SFLOW_H
#define SST_STRM_SFLOW_H

#include <QHash>
#include <QList>
#include <QQueue>

#include "../flow.h"	// XXX
#include "strm/base.h"

namespace SST {

class StreamPeer;
class StreamResponder;
class StreamAttachment;
class StreamTxAttachment;
class StreamRxAttachment;

class StreamFlow : public Flow, public StreamProtocol
{
	friend class BaseStream;
	friend class StreamPeer;
	friend class StreamResponder;
	friend class StreamTxAttachment;
	friend class StreamRxAttachment;
	Q_OBJECT

	typedef StreamAttachment Attachment;
	typedef StreamTxAttachment TxAttachment;
	typedef StreamRxAttachment RxAttachment;
	typedef StreamTxRec TxRec;

private:
	// StreamPeer this flow is associated with.
	// A StreamFlow is always either a direct child of its StreamPeer,
	// or a child of a KeyInitiator which is a child of its StreamPeer,
	// so there should be no chance of this pointer ever dangling.
	StreamPeer *peer;

	// Top-level stream used for connecting to services
	BaseStream root;

	// Hash table of active streams indexed by stream ID
	QHash<StreamId, TxAttachment*> txsids;	// Our SID namespace
	QHash<StreamId, RxAttachment*> rxsids;	// Peer's SID namespace
	StreamCtr txctr;			// Next StreamCtr to assign
	StreamCtr txackctr;			// Last StreamCtr acknowledged
	StreamCtr rxctr;			// Last StreamCtr received

	// List of closed stream IDs waiting for close acknowledgment
	QList<StreamId> closed;

	// Round-robin queue of Streams with packets waiting to transmit
	// XX would prefer a smarter scheduling algorithm, e.g., stride.
	QQueue<BaseStream*> tstreams;

//	// Round-robin queue of TxAttachments waiting to transmit
//	// XX would prefer a smarter scheduling algorithm, e.g., stride.
//	QQueue<TxAttachment*> tattq;

	// Packets transmitted and waiting for acknowledgment,
	// indexed by assigned transmit sequence number.
	QHash<qint64,BaseStream::Packet> ackwait;

	// Packets already presumed lost ("missed")
	// but still waiting for potential acknowledgment until expiry.
	QHash<qint64,BaseStream::Packet> expwait;

	// RxSID of stream on which we last received a packet -
	// this determines for which stream we send receive window info
	// when transmitting "bare" Ack packets.
	StreamId acksid;


	// Attach a stream to this flow, allocating a SID for it if necessary.
	//StreamId attach(BaseStream *bs, StreamId sid = 0);
	//void detach(BaseStream *bs);

	StreamFlow(Host *h, StreamPeer *peer, const QByteArray &peerid);
	~StreamFlow();

	inline StreamPeer *target() { return peer; }

	inline int dequeueStream(BaseStream *strm)
		{ return tstreams.removeAll(strm); }
	void enqueueStream(BaseStream *strm);

	// Detach all streams currently transmit-attached to this stream,
	// and send any of their outstanding packets back for retransmission.
	void detachAll();

	// Override Flow's default transmitAck() method
	// to include stream-layer info in explicit ack packets.
	virtual bool transmitAck(QByteArray &pkt,
				quint64 ackseq, unsigned ackct);

	virtual bool flowReceive(qint64 rxseq, QByteArray &pkt);
	virtual void acked(quint64 txseq, int npackets, quint64 rxseq);
	virtual void missed(quint64 txseq, int npackets);
	virtual void expire(quint64 txseq, int npackets);

	virtual void start(bool initiator);
	virtual void stop();

private slots:
	void gotReadyTransmit();
	void gotLinkStatusChanged(LinkStatus newstatus);
};

} // namespace SST

#endif	// SST_STRM_SFLOW_H
