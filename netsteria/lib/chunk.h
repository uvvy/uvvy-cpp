#ifndef ARCH_CHUNK_H
#define ARCH_CHUNK_H

#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QDataStream>
#include <QPointer>
#include <QTimer>

#include "util.h"
#include "peer.h"

class SST::XdrStream;
class SST::Stream;
class AbstractChunkReader;
class ChunkShare;
class ChunkPeer;


// Basic format defs - should be moved into a C-only header
#define VXA_ARCHSIG	0x56584168	// 'VXAh'
#define VXA_CHUNKSIG	0x56584163	// 'VXAc'

#define VXA_IHASH_SIZE	32	// Size of inner hash (encrypt key)
#define VXA_OHASH_SIZE	64	// Size of outer hash
#define VXA_OCHECK_SIZE	8	// Size of outer hash check
#define VXA_RECOG_SIZE	32	// Size of recognizer preamble


// A 64-bit outer hash check is simply
// the first 8 bytes of a chunk's 512-bit outer hash.
typedef quint64 OuterCheck;


// Extract the 64-bit check field from a full outer hash.
OuterCheck hashCheck(const QByteArray &ohash);


// Base pseudo-class to provide symbols shared by several Chunk classes
struct ChunkProtocol
{
	// Chunk message codes
	enum MsgCode {
		ChunkStatusRequest	= 0x101,
		ChunkRequest		= 0x102,

		ChunkStatusReply	= 0x201,
		ChunkReply		= 0x202,
	};

	// Chunk status codes
	enum ChunkStatus {
		InvalidStatus = 0,
		OnlineChunk,		// Have it immediately available
		OfflineChunk,		// Might be available with user effort
		UnknownChunk,		// Don't know anything about it
		LostChunk,		// Used to have it, but lost
	};


private:
	friend class ChunkShare;
	friend class ChunkPeer;

	struct Request {
		const QByteArray ohash;		// Chunk identity

		SST::QPointerSet<AbstractChunkReader> readers;	// who wants it

		// Potential peers that might have this chunk.
		// A peer is included if we've sent it a status request,
		// or if it has positively reported having the chunk.
		QSet<ChunkPeer*> potentials;

		// Who knows what about this chunk.
		// We use the 'InvalidStatus' code here to mean
		// we've requested status from this peer but not received it.
		QHash<ChunkPeer*, ChunkStatus> status;


		inline Request(const QByteArray &ohash) : ohash(ohash) { }
	};
};


class AbstractChunkReader : public QObject
{
	friend class ChunkPeer;
	friend class ChunkShare;

protected:
	inline AbstractChunkReader(QObject *parent = NULL)
		: QObject(parent) { }

	// Submit a request to read a chunk.
	// For each such chunk request this class will send either
	// a gotData() or a noData() signal for the requested chunk
	// (unless the ChunkReader gets destroyed first).
	// The caller can use a single ChunkReader object
	// to read multiple chunks simultaneously.
	void readChunk(const QByteArray &ohash);


	// The chunk reader calls these methods when a chunk arrives,
	// or when it gives up on finding a chunk.
	virtual void gotData(const QByteArray &ohash,
				const QByteArray &data) = 0;
	virtual void noData(const QByteArray &ohash) = 0;
};

class ChunkReader : public AbstractChunkReader
{
	Q_OBJECT

	inline ChunkReader(QObject *parent = NULL)
		: AbstractChunkReader(parent) { }

signals:
	void gotData(const QByteArray &ohash, const QByteArray &data);
	void noData(const QByteArray &ohash);
};

// Single-instance class that handles chunk sharing in the social network.
class ChunkShare : public PeerService, ChunkProtocol
{
	friend class ChunkPeer;
	Q_OBJECT

private:
	// Outstanding chunk requests
	static QHash<QByteArray, Request*> requests;

	// Maintain a persistent chunk query stream for each of our peers.
	static QHash<QByteArray, ChunkPeer*> peers;


	ChunkShare();

public:
	static ChunkShare *instance();

	static inline void init() { (void)instance(); }

	static void requestChunk(AbstractChunkReader *reader,
				const QByteArray &ohash);

private:
	static ChunkPeer *peer(const QByteArray &hostid, bool create);

	// Check all peers for chunks they might be able to download.
	static void checkPeers();

	// Check a particular request to see if it can still be satisfied;
	// otherwise delete it and send noData() to all requestors.
	static void checkRequest(Request *req);

private slots:
	void gotOutStreamConnected(Stream *stream);
	void gotOutStreamDisconnected(Stream *stream);
	void gotInStreamConnected(Stream *stream);
};

// Private helper class for ChunkShare:
// implements our interaction with a given peer for chunk sharing.
class ChunkPeer : public QObject, public ChunkProtocol
{
	friend class ChunkShare;
	Q_OBJECT

private:
	// Minimum delay between successive connection attempts - 1 minute
	static const int reconDelay = 60*1000;

	const QByteArray hostid;	// Host ID of peer
	QSet<QByteArray> wishlist;	// What we want from this host
	QByteArray current;		// Ohash of chunk being gotten


	ChunkPeer(const QByteArray &hostid);
	~ChunkPeer();

	inline QByteArray peerHostId() { return hostid; }
	QString peerName();

	void checkWork();

	void sendStatusRequest(Request *req);

	void gotStatusRequest(SST::Stream *strm, SST::XdrStream &rs);
	void gotChunkRequest(SST::Stream *strm, SST::XdrStream &rs);

	void gotStatusReply(SST::Stream *strm, SST::XdrStream &rs);
	void gotChunkReply(SST::Stream *strm, SST::XdrStream &rs);

	void removeFromRequests();

private slots:
	void peerReadMessage();
};


#endif	// ARCH_CHUNK_H
