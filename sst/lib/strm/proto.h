#ifndef SST_STRM_PROTO_H
#define SST_STRM_PROTO_H

#include <QPair>

#include "flow.h"
#include "xdr.h"


namespace SST {


// A few types in the main SST scope
typedef quint16 StreamId;		// Stream ID within channel
typedef quint32 StreamSeq;		// Stream byte sequence number
typedef quint64 StreamCtr;		// Counter for SID assignment


// Unique stream identifier - identifiers streams across channels
// XX should contain a "method identifier" of some kind?
// XX also, perhaps a more compact encoding than plain XDR?
struct UniqueStreamId {
	StreamCtr streamCtr;		// Stream counter in channel
	QByteArray chanId;		// Unique channel+direction ID

	inline UniqueStreamId() { chanId.clear(); streamCtr = 0; }
	inline UniqueStreamId(StreamCtr streamCtr, QByteArray chanId)
		: streamCtr(streamCtr), chanId(chanId) { }

	inline bool isNull() const { return chanId.isNull(); }

	inline bool operator==(const UniqueStreamId &o) const
		{ return streamCtr == o.streamCtr && chanId == o.chanId; }
};

inline XdrStream &operator<<(XdrStream &xs, const UniqueStreamId &usid)
	{ xs << usid.streamCtr << usid.chanId; return xs; }
inline XdrStream &operator>>(XdrStream &xs, UniqueStreamId &usid)
	{ xs >> usid.streamCtr >> usid.chanId; return xs; }

inline QDebug &operator<<(QDebug &debug, const UniqueStreamId &usid) {
	debug.nospace() << "USID[" << usid.chanId.toBase64()
		<< ":" << usid.streamCtr << "]";
	return debug.space();
}



/** @internal SST stream protocol definitions.
 * This class simply provides SST protcol definition constants
 * for use in the other Stream classes below.
 */
class StreamProtocol
{
public:
	// Control chunk magic value for the structured stream transport.
	// 0x535354 = 'SST': 'Structured Stream Transport'
	static const quint32 magic = 0x00535354;

	// Maximum transmission unit. XX should be dynamic.
	static const int mtu = 1200;

	// Minimum receive buffer size. XX should be dynamically based on mtu.
	static const int minReceiveBuffer = mtu*2;

	// Maximum size of datagram to send using the stateless optimization.
	// XX should be dynamic.
	static const int maxStatelessDatagram = mtu * 4;

	// Sizes of various stream header types
	static const int hdrlenMin		= Flow::hdrlen + 4;
	static const int hdrlenInit		= Flow::hdrlen + 8;
	static const int hdrlenReply		= Flow::hdrlen + 8;
	static const int hdrlenData		= Flow::hdrlen + 8;
	static const int hdrlenDatagram		= Flow::hdrlen + 4;
	static const int hdrlenReset		= Flow::hdrlen + 4;
	static const int hdrlenAttach		= Flow::hdrlen + 4;
	static const int hdrlenAck		= Flow::hdrlen + 4;

	// Header layouts
	struct StreamHeader {
		quint16	sid;
		quint8	type;
		quint8	win;
	};

	// The Type field is divided into a 4-bit major type
	// and a 4-bit subtype/flags field.
	static const unsigned typeBits		= 4;
	static const unsigned typeMask		= (1 << typeBits) - 1;
	static const unsigned typeShift		= 4;
	static const unsigned subtypeBits	= 4;
	static const unsigned subtypeMask	= (1 << subtypeBits) - 1;
	static const unsigned subtypeShift	= 0;

	// Major packet type codes (4 bits)
	enum PacketType {
		InvalidPacket	= 0x0,		// Always invalid
		InitPacket	= 0x1,		// Initiate new stream
		ReplyPacket	= 0x2,		// Reply to new stream
		DataPacket	= 0x3,		// Regular data packet
		DatagramPacket	= 0x4,		// Best-effort datagram
		AckPacket	= 0x5,		// Explicit acknowledgment
		ResetPacket	= 0x6,		// Reset stream
		AttachPacket	= 0x7,		// Attach stream
		DetachPacket	= 0x8,		// Detach stream
	};

	// The Window field consists of some flags and a 5-bit exponent.
	static const unsigned winSubstreamFlag	= 0x80;	// Substream window
	static const unsigned winInheritFlag	= 0x40;	// Inherited window
	static const unsigned winExpBits	= 5;
	static const unsigned winExpMask	= (1 << winExpBits) - 1;
	static const unsigned winExpMax		= winExpMask;
	static const unsigned winExpShift	= 0;

	struct InitHeader : public StreamHeader {
		quint16 rsid;			// New Stream ID
		quint16 tsn;			// 16-bit transmit seq no
	};
	typedef InitHeader ReplyHeader;

	struct DataHeader : public StreamHeader {
		quint32 tsn;			// 32-bit transmit seq no
	};

	typedef StreamHeader DatagramHeader;
	typedef StreamHeader AckHeader;
	typedef StreamHeader ResetHeader;
	typedef StreamHeader AttachHeader;
	typedef StreamHeader DetachHeader;


	// Subtype/flag bits for Init, Reply, and Data packets
	static const quint8 dataPushFlag	= 0x4;	// Push to application
	static const quint8 dataMessageFlag	= 0x2;	// End of message
	static const quint8 dataCloseFlag	= 0x1;	// End of stream
	static const quint8 dataAllFlags	= 0x7;	// All signal flags

	// Flag bits for Datagram packets
	static const quint8 dgramBeginFlag	= 0x2;	// First fragment
	static const quint8 dgramEndFlag	= 0x1;	// Last fragment

	// Flag bits for Attach packets
	static const quint8 attachInitFlag	= 0x8;	// Initiate stream
	static const quint8 attachSlotMask	= 0x1;	// Slot to use

	// Flag bits for Reset packets
	static const quint8 resetDirFlag	= 0x1;	// SID orientation


	// StreamId 0 always refers to the root stream.
	static const StreamId sidRoot = 0x0000;


	// Index values for [dir] dimension in attach array
	enum AttachDir {
		Local = 0,		// my SID space
		Remote = 1,		// peer's SID space
	};

	// Number of redundant attachment points per stream
	static const int maxAttach = 2;

	// Maximum number of in-use SIDs to skip while trying to allocate one,
	// before we just give up and detach an existing one in this range.
	static const int maxSidSkip = 16;


	// Service message codes
	enum ServiceCode {
		ConnectRequest	= 0x101,	// Connect to named service
		ConnectReply	= 0x201,	// Response to connect request
	};

	// Maximum size of a service request or response message
	static const int maxServiceMsgSize = 1024;

	// Service/protocol pairs used to index registered StreamServers.
	typedef QPair<QString,QString> ServicePair;


private:
	friend class Stream;
	friend class StreamFlow;
	friend class StreamResponder;
};

} // namespace SST

inline uint qHash(const SST::StreamProtocol::ServicePair &svpair)
	{ return qHash(svpair.first) + qHash(svpair.second); }

inline uint qHash(const SST::UniqueStreamId &usid)
	{ return qHash(usid.streamCtr) + qHash(usid.chanId); }

#endif	// SST_STRM_PROTO_H
