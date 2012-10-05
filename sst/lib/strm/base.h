#ifndef SST_STRM_BASE_H
#define SST_STRM_BASE_H

#include <QSet>
#include <QQueue>
#include <QPointer>

#include "stream.h"
#include "strm/abs.h"

namespace SST {

class Stream;
class StreamFlow;
class StreamPeer;
class BaseStream;


// Helper struct representing an attachment point on a stream
// where the stream attaches to a flow/channel.
struct StreamAttachment : public StreamProtocol {
	BaseStream	*strm;		// Stream we're a part of
	StreamFlow	*flow;		// Flow we're attached to
	StreamId	sid;		// Stream ID in flow
	//quint64	initseq;	// PktSeq of first Init
	quint64		sidseq;		// Reference PktSeq for SID

	inline StreamAttachment() { flow = NULL; }
};

struct StreamTxAttachment : public StreamAttachment
{
	bool		active;		// Currently active and usable
	bool		deprecated;	// Opening a replacement channel

	inline StreamTxAttachment() { active = deprecated = false; }

	inline bool isInUse() { return flow != NULL; }
	inline bool isAcked() { return sidseq != maxPacketSeq; }
	inline bool isActive() { return active; }
	inline bool isDeprecated() { return deprecated; }

	// Transition from Unused to Attaching -
	// this happens when we send a first Init, Reply, or Attach packet.
	void setAttaching(StreamFlow *flow, quint16 sid);

	// Transition from Attaching to Active -
	// this happens when we get an Ack to our Init, Reply, or Attach.
	inline void setActive(PacketSeq rxseq) {
		Q_ASSERT(isInUse() && !isAcked());
		this->sidseq = rxseq;
		this->active = true;
	}

	// Transition to the unused state.
	void clear();
};

struct StreamRxAttachment : public StreamAttachment
{
	inline bool isActive() { return flow != NULL; }

	// Transition from unused to active.
	void setActive(StreamFlow *flow, quint16 sid, PacketSeq rxseq);

	// Transition to the unused state.
	void clear();
};

/** @internal Transmit record for a packet transmitted on a stream.
 */
struct StreamTxRec
{
	BaseStream *strm;
	qint32 bsn;
	qint8 type;

	inline StreamTxRec(BaseStream *strm, qint32 bsn, qint8 type)
		: strm(strm), bsn(bsn), type(type) { }
};

#if 0
/** @internal Flow control state node for SST streams.
 * May be shared by multiple streams having a hereditary relationship.
 */
struct StreamTxFlowState
{
	qint32	refs;		// Number of streams sharing this flow state

	// Byte-stream flow control
	qint32	bwin;		// Current receiver-specified window

	// Child stream flow control
};
#endif


/** @internal Basic internal stream control object.
 * The separation between the internal stream control object
 * and the application-visible Stream object is primarily needed
 * so that SST can hold onto a stream's state and gracefully shut it down
 * after the application deletes its Stream object representing it.
 * This separation also keeps the internal stream control variables
 * out of the public C++ API header files and thus able to change
 * without breaking binary compatibility,
 * and makes it easy to implement service/protocol negotiation
 * for top-level application streams by extending BaseStream (see AppStream).
 *
 * @see Stream, AppStream
 */
class BaseStream : public AbstractStream
{
	friend class Stream;
	friend class StreamFlow;
	friend class StreamPeer;
	friend class StreamTxAttachment;
	friend class StreamRxAttachment;
	Q_OBJECT

private:
	enum State {
		Fresh = 0,		// Newly created
		WaitService,		// Initiating, waiting for svc reply
		Accepting,		// Accepting, waiting for svc request
		Connected,		// Connection established
		Disconnected		// Connection terminated
	};

	struct Packet {
		BaseStream *strm;
		//qint64 txseq;			// Transmit sequence number
		qint64 tsn;			// Logical byte position
		QByteArray buf;			// Packet buffer incl. headers
		int hdrlen;			// Size of flow + stream hdrs
		PacketType type;		// Type of packet
		bool late;			// on ackwait and presumed lost

		inline Packet() : strm(NULL), type(InvalidPacket) { }
		inline Packet(BaseStream *strm, PacketType type)
			: strm(strm), type(type) { }

		inline bool isNull() const { return strm == NULL; }
		inline int payloadSize() const
			{ return buf.size() - hdrlen; }
	};

	struct RxSegment {
		qint32 rsn;			// Logical byte position
		QByteArray buf;			// Packet buffer incl. headers
		int hdrlen;			// Size of flow + stream hdrs

		inline int segmentSize() const
			{ return buf.size() - hdrlen; }

		inline StreamHeader *hdr()
			{ return (StreamHeader*)(buf.data() + Flow::hdrlen); }
		inline const StreamHeader *constHdr() const
			{ return (const StreamHeader*)
					(buf.constData() + Flow::hdrlen); }

		inline quint8 flags() const
			{ return constHdr()->type & dataAllFlags; }
		inline bool hasFlags() const
			{ return flags() != 0; }
	};

	typedef StreamAttachment Attachment;
	typedef StreamTxAttachment TxAttachment;
	typedef StreamRxAttachment RxAttachment;
	typedef StreamTxRec TxRec;


	/// Default receive buffer size for new top-level streams
	//static const int defaultReceiveBuffer = minReceiveBuffer;
	static const int defaultReceiveBuffer = 65536;


	// Connection state
	StreamPeer	*peer;			// Our peer, if usid not Null
	UniqueStreamId	usid;			// Our UniqueStreamId, or null
	UniqueStreamId	pusid;			// Parent's UniqueStreamId
	QPointer<BaseStream> parent;		// Parent, if it still exists
	State		state;
	bool		init;			// Initiating, not yet acked
	bool		toplev;			// This is a top-level stream
	//bool		mature;			// Seen at least one round-trip
	bool		endread;		// Seen or forced EOF on read
	bool		endwrite;		// We've written our EOF marker

	// Flow attachment state
	TxAttachment	tatt[maxAttach];	// Our channel attachments
	RxAttachment	ratt[maxAttach];	// Peer's channel attachments
	TxAttachment	*tcuratt;		// Current transmit-attachment

	// Byte transmit state
	qint32		tasn;			// Next transmit BSN to assign
	qint32		twin;			// Current transmit window
	qint32		tflt;			// Bytes currently in flight
	bool		tqflow;			// We're on flow's tx queue
	QSet<qint32>	twait;			// Segments waiting to be ACKed
	QQueue<Packet>	tqueue;			// Packets to be transmitted
	qint32		twaitsize;		// Bytes in twait segments

	// Substream transmit state
	qint32		tswin;			// Transmit substream window
	qint32		tsflt;			// Outstanding substreams

	// Byte-stream receive state
	qint32		rsn;			// Next SSN expected to arrive
	qint32		ravail;			// Received bytes available
	qint32		rmsgavail;		// Bytes avail in cur message
	qint32		rbufused;		// Total buffer space used
	quint8		rwinbyte;		// Receive window log2
	QList<RxSegment> rahead;		// Received out of order
	QQueue<RxSegment> rsegs;		// Received, waiting to be read
	QQueue<qint64>	rmsgsize;		// Sizes of received messages
	qint32		rcvbuf;			// Recv buf size for flow ctl
	qint32		crcvbuf;		// Recv buf for child streams

	// Substream receive state
	QQueue<AbstractStream*> rsubs;		// Received, waiting substreams


private:
	// Clear out this stream's state as if preparing for deletion,
	// without actually deleting the object yet.
	void clear();

	// Connection
	void gotServiceReply();
	void gotServiceRequest();

	// Actively initiate a transmit-attachment
	void tattach();
	void setUsid(const UniqueStreamId &usid);

	// Data transmission
	void txenqueue(const Packet &pkt);	// Queue a pkt for transmission
	void txenqflow(bool immed = false);
	//void txPrepare(Packet &pkt, StreamFlow *flow);
	void transmit(StreamFlow *flow);
	void txAttachData(PacketType type, StreamId refsid);
	void txData(Packet &p);
	void txDatagram();
	void txAttach();
	static void txReset(StreamFlow *flow, quint16 sid, quint8 flags);

	// Data reception
	static bool receive(qint64 pktseq, QByteArray &pkt, StreamFlow *flow);
	static bool rxInitPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxReplyPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxDataPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxDatagramPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxAckPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxResetPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxAttachPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	static bool rxDetachPacket(quint64 pktseq, QByteArray &pkt,
				StreamFlow *flow);
	void rxData(QByteArray &pkt, quint32 byteseq);

	BaseStream *rxSubstream(quint64 pktseq, StreamFlow *flow,
				quint16 sid, unsigned slot,
				const UniqueStreamId &usid);

	// Return the next receive window update byte
	// for some packet we are transmitting on this stream.
	// XX alternate between byte-window and substream-window updates.
	inline quint8 receiveWindow() { return rwinbyte; }
	void calcReceiveWindow();

	void calcTransmitWindow(quint8 win);

	// StreamFlow calls these to return our transmitted packets to us
	// after being held in ackwait.
	// The missed() method returns true
	// if the flow should keep track of the packet until it expires,
	// at which point it calls expire() and unconditionally removes it.
	void acked(StreamFlow *flow, const Packet &pkt, quint64 rxseq);
	bool missed(StreamFlow *flow, const Packet &pkt);
	void expire(StreamFlow *flow, const Packet &pkt);

	void endflight(const Packet &pkt);

	// Disconnect and set an error condition.
	void fail(const QString &err);


private slots:
	// We connect this to the readyReadMessage() signals
	// of any substreams queued in our rsubs list waiting to be accepted,
	// in order to forward the indication to the client
	// via the parent stream's readyReadDatagram() signal.
	void subReadMessage();

	// We connect this signal to our StreamPeer's flowConnected()
	// while waiting for a flow to attach to.
	void gotFlowConnected();

	// We connect this signal to our parent stream's attached() signal
	// while we're waiting for it to attach so we can init.
	void gotParentAttached();


public:
	/** Create a BaseStream instance.
	 * @param peerid the endpoint identifier (EID) of the remote host
	 *		with which this stream will be used to communicate.
	 *		The destination may be either a cryptographic EID
	 * 		or a non-cryptographic legacy address
	 *		as defined by the Ident class.
	 * @param parent the parent stream, or NULL if none (yet).
	 */
	BaseStream(Host *host, QByteArray peerid, BaseStream *parent);
	virtual ~BaseStream();

	/** Connect to a given service on a remote host.
	 * @param service the service name to connect to on the remote host.
	 *		This parameter replaces the port number
	 *		that TCP traditionally uses to differentiate services.
	 * @param dstep	an optional location hint
	 *		for SST to use in attempting to contact the host.
	 *		If the dstid parameter is a cryptographic EID,
	 *		which is inherently location-independent,
	 *		SST may need a location hint to find the remote host
	 *		if this host and the remote host are not currently
	 *		registered at a common registration server,
	 *		for example.
	 *		This parameter is not needed
	 *		if the dstid is a non-cryptographic legacy address.
	 * @see Ident
	 */
	void connectTo(const QString &service, const QString &protocol);

	/// Returns true if the underlying link is currently connected
	/// and usable for data transfer.
	inline bool isLinkUp()
		{ return state == Connected; }

	/** Set the stream's transmit priority level.
	 * This method overrides AbstractStream's default method
	 * to move the stream to the correct transmit queue if necessary.
	 */
	void setPriority(int pri);

	// Implementations of AbstractStream's data I/O methods
	virtual qint64 bytesAvailable() const { return ravail; }
	virtual qint64 bytesToWrite() const { return twaitsize; } // XXX dgrams
	virtual int readData(char *data, int maxSize);
	virtual int writeData(const char *data, int maxSize,
				quint8 endflags);

	virtual int pendingMessages() const
		{ return rmsgsize.size(); }
	virtual qint64 pendingMessageSize() const
		{ return hasPendingMessages() ? rmsgsize.at(0) : -1; }
	virtual int readMessage(char *data, int maxSize);
	virtual QByteArray readMessage(int maxSize);

	virtual bool atEnd() const { return endread; }

	virtual void setReceiveBuffer(int size);
	virtual void setChildReceiveBuffer(int size);


	////////// Substreams //////////

	// Initiate or accept substreams
	virtual AbstractStream *openSubstream();
	virtual AbstractStream *acceptSubstream();

	// Send and receive unordered, unreliable datagrams on this stream.
	virtual int writeDatagram(const char *data, int size, bool reliable);
	AbstractStream *getDatagram();
	virtual int readDatagram(char *data, int maxSize);
	virtual QByteArray readDatagram(int maxSize);

	void shutdown(Stream::ShutdownMode mode);

	/// Immediately reset a stream to the disconnected state.
	/// Outstanding buffered data may be lost.
	void disconnect();

	/// Dump the state of this stream, for debugging purposes.
	void dump();


signals:
	// A complete message has been received.
	void readyReadMessage();

	// An active attachment attempt succeeded and was acked by receiver.
	void attached();

	// An active detachment attempt succeeded and was acked by receiver.
	void detached();
};

} // namespace SST

#endif	// SST_STRM_BASE_H
