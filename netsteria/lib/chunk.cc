
#include <QtDebug>

#include "chunk.h"
#include "store.h"
#include "stream.h"
#include "host.h"
#include "xdr.h"

using namespace SST;


OuterCheck hashCheck(const QByteArray &ohash)
{
	Q_ASSERT(ohash.size() == VXA_OHASH_SIZE);

	XdrStream ds(ohash);
	OuterCheck chk;
	ds >> chk;
	return chk;
}


////////// AbstractChunkReader //////////

void AbstractChunkReader::readChunk(const QByteArray &ohash)
{
	QByteArray data = Store::readStores(ohash);
	if (!data.isEmpty())
		return gotData(ohash, data);

	// Request asynchronously from other nodes
	ChunkShare::requestChunk(this, ohash);
}


////////// ChunkShare //////////

QHash<QByteArray, ChunkShare::Request*> ChunkShare::requests;
QHash<QByteArray, ChunkPeer*> ChunkShare::peers;

ChunkShare::ChunkShare()
:	PeerService("Data", tr("Data sharing"),
			"NstData", tr("Netsteria data sharing protocol"))
{
	connect(this, SIGNAL(outStreamConnected(Stream *)),
		this, SLOT(gotOutStreamConnected(Stream *)));
	connect(this, SIGNAL(outStreamDisconnected(Stream *)),
		this, SLOT(gotOutStreamDisconnected(Stream *)));
	connect(this, SIGNAL(inStreamConnected(Stream *)),
		this, SLOT(gotInStreamConnected(Stream *)));
}

ChunkShare *ChunkShare::instance()
{
	static ChunkShare *obj;
	if (!obj)
		obj = new ChunkShare();
	return obj;
}

ChunkPeer *ChunkShare::peer(const QByteArray &hostid, bool create)
{
	if (!create)
		return peers.value(hostid);

	ChunkPeer *&peer = peers[hostid];
	if (!peer && create)
		peer = new ChunkPeer(hostid);
	return peer;
}

void ChunkShare::requestChunk(AbstractChunkReader *reader,
				const QByteArray &ohash)
{
	qDebug() << "ChunkShare: searching for chunk" << ohash.toBase64();

	// Make sure we're set up
	init();

	// Find or create a Request for this chunk
	Request *&req = requests[ohash];
	if (!req)
		req = new Request(ohash);
	req->readers.insert(reader);

	// Send a chunk status request to each of our current peers
	foreach (ChunkPeer *peer, peers)
		peer->sendStatusRequest(req);
}

void ChunkShare::checkRequest(Request *req)
{
	Q_ASSERT(req);

	if (!req->potentials.isEmpty())
		return;		// Still have places to look

	// Remove the request first,
	// in case a reader immediately adds new ones below.
	Q_ASSERT(requests.value(req->ohash) == req);
	requests.remove(req->ohash);

	// Signal each reader who's waiting for the chunk
	foreach (AbstractChunkReader *reader, req->readers) {
		if (reader == NULL) {
			qDebug() << "Reader disappeared while reading chunk";
			continue;
		}
		reader->noData(req->ohash);
	}

	// Delete the request
	delete req;
}

void ChunkShare::checkPeers()
{
	// Make sure each peer has something useful to do
	foreach (ChunkPeer *peer, peers)
		peer->checkWork();
}

void ChunkShare::gotOutStreamConnected(Stream *stream)
{
	// Find or create the appropriate ChunkPeer
	ChunkPeer *peer = this->peer(stream->remoteHostId(), true);

	// Watch for messages on this stream
	connect(stream, SIGNAL(readyReadMessage()),
		peer, SLOT(peerReadMessage()));

	// Reset current chunk downloading state for this peer,
	// since any chunk we were downloading probably got lost
	// when our last outgoing connection (if any) failed.
	peer->current = QByteArray();

	// Unconditionally re-send all outstanding status requests.
	foreach (Request *req, ChunkShare::requests)
		peer->sendStatusRequest(req);
}

void ChunkShare::gotOutStreamDisconnected(Stream *stream)
{
	ChunkPeer *peer = this->peer(stream->remoteHostId(), false);
	if (!peer)
		return;

	// If the primary stream has been disconnected,
	// remove this peer from all outstanding requests;
	// it obviously won't be able to do much for us at the moment.
	peer->removeFromRequests();
}

void ChunkShare::gotInStreamConnected(Stream *stream)
{
	// Find or create the appropriate ChunkPeer
	ChunkPeer *peer = this->peer(stream->remoteHostId(), true);

	// Watch for messages on this stream
	connect(stream, SIGNAL(readyReadMessage()),
		peer, SLOT(peerReadMessage()));
}


////////// ChunkPeer //////////

ChunkPeer::ChunkPeer(const QByteArray &hostid)
:	hostid(hostid)
{
}

ChunkPeer::~ChunkPeer()
{
	// Remove any references to us from all outstanding chunk requests
	removeFromRequests();
}

QString ChunkPeer::peerName()
{
	return ChunkShare::instance()->peerTable()->name(peerHostId());
}

void ChunkPeer::removeFromRequests()
{
	foreach (Request *req, ChunkShare::requests) {
		req->potentials.remove(this);
		req->status.remove(this);
		ChunkShare::checkRequest(req);
	}
}

void ChunkPeer::sendStatusRequest(Request *req)
{
	Stream *stream = ChunkShare::instance()->connectToPeer(hostid);
	Q_ASSERT(stream);
	if (!stream->isConnected()) {
		qDebug() << "Peer" << peerName()
			<< "is unreachable for chunk status request";
		return;
	}

	qDebug() << "Requesting chunk status from" << peerName();

	// Build a chunk status request message to find out who has the chunk
	QByteArray msg;
	XdrStream ws(&msg, QIODevice::WriteOnly);
	ws << (qint32)ChunkStatusRequest << req->ohash;

	// Send it
	stream->writeMessage(msg);

	// Remember that we sent the status request
	// XX overwrites any previous status - correct?
	req->potentials.insert(this);
	req->status.insert(this, InvalidStatus);
}

void ChunkPeer::checkWork()
{
	Stream *stream = ChunkShare::instance()->connectToPeer(hostid);
	if (!stream->isConnected())
		return;		// Unconnected peers can't do work
	if (!current.isEmpty())
		return;		// Already busy downloading a chunk
	if (wishlist.isEmpty())
		return;		// Nothing to do

	// Find a chunk on this peer's wishlist
	// that no one else is downloading at the moment.
	QByteArray selhash;
	foreach (const QByteArray &ohash, wishlist) {

		// Make sure this chunk is still actually needed.
		if (ChunkShare::requests.value(ohash) == NULL) {
			wishlist.remove(ohash);
			continue;
		}

		// Make sure no one else is already downloading it.
		bool found = false;
		foreach (ChunkPeer *p2, ChunkShare::peers) {
			if (p2->current == ohash) {
				found = true;
				break;
			}
		}
		if (!found) {
			selhash = ohash;
			break;
		}
	}
	if (selhash.isEmpty()) {
		qDebug() << "Nothing non-redundant for peer"
			<< peerName() << "to do.";
		return;
	}

	// Request the selected chunk from this peer
	qDebug() << "Requesting chunk" << selhash.toBase64()
		<< "from peer" << peerName();
	QByteArray msg;
	XdrStream ws(&msg, QIODevice::WriteOnly);
	ws << (qint32)ChunkRequest << selhash;
	stream->writeMessage(msg);

	// Remember that we're waiting for this chunk
	current = selhash;
}

void ChunkPeer::peerReadMessage()
{
	Stream *strm = (Stream*)sender();
	Q_ASSERT(strm);

	forever {
		if (!strm->isLinkUp() || strm->atEnd()) {
			qDebug() << "ChunkPeer:" << peerName()
				<< "disconnected";
			strm->disconnect();
			break;
		}

		QByteArray msg = strm->readMessage();
		if (msg.isEmpty())
			break;

		XdrStream rs(&msg, QIODevice::ReadOnly);
		qint32 msgcode;
		rs >> msgcode;
		switch ((MsgCode)msgcode) {
		case ChunkStatusRequest:
			gotStatusRequest(strm, rs);
			break;
		case ChunkRequest:
			gotChunkRequest(strm, rs);
			break;
		case ChunkStatusReply:
			gotStatusReply(strm, rs);
			break;
		case ChunkReply:
			gotChunkReply(strm, rs);
			break;
		default:
			qDebug() << "ChunkShare: unknown message code"
				<< msgcode;
			break;
		}
	}
}

void ChunkPeer::gotStatusRequest(Stream *stream, XdrStream &rs)
{
	QByteArray ohash;
	rs >> ohash;
	if (rs.status() != rs.Ok || ohash.isEmpty()) {
		qDebug("Invalid chunk status request message");
		return;
	}

	// XX add proper status check mechanism
	QByteArray data = Store::readStores(ohash);
	ChunkStatus st = data.isEmpty() ? UnknownChunk : OnlineChunk;

	// Build and send the reply
	QByteArray msg;
	XdrStream ws(&msg, QIODevice::WriteOnly);
	ws << (qint32)ChunkStatusReply << ohash << (qint32)st;
	stream->writeMessage(msg);
}

void ChunkPeer::gotChunkRequest(Stream *stream, XdrStream &rs)
{
	QByteArray ohash;
	rs >> ohash;
	if (rs.status() != rs.Ok || ohash.isEmpty()) {
		qDebug("Invalid chunk request message");
		return;
	}

	QByteArray data = Store::readStores(ohash);

	// Build and send the reply
	QByteArray msg;
	XdrStream ws(&msg, QIODevice::WriteOnly);
	ws << (qint32)ChunkReply << ohash << data;
	stream->writeMessage(msg);
}

void ChunkPeer::gotStatusReply(Stream *, XdrStream &rs)
{
	QByteArray ohash;
	qint32 st;
	rs >> ohash >> st;
	if (rs.status() != rs.Ok || ohash.isEmpty()) {
		qDebug("Invalid chunk status reply message");
		return;
	}

	Request *req = ChunkShare::requests.value(ohash);
	if (!req) {
		qDebug("Got useless chunk status reply for undesired chunk");
		return;
	}

	req->status.insert(this, (ChunkStatus)st);
	switch ((ChunkStatus)st) {
	case OnlineChunk:
		wishlist.insert(ohash);
		ChunkShare::checkPeers();
		break;
	default:
		qDebug() << "Got unknown chunk status" << st;
		// fall through...
	case UnknownChunk:
		req->potentials.remove(this);
		ChunkShare::checkRequest(req);
		break;
	}
}

void ChunkPeer::gotChunkReply(Stream *, XdrStream &rs)
{
	QByteArray ohash, data;
	rs >> ohash >> data;
	if (rs.status() != rs.Ok || ohash.isEmpty()) {
		qDebug("Invalid chunk status reply message");
		return;
	}

	Request *req = ChunkShare::requests.value(ohash);
	if (!req) {
		qDebug("Got useless chunk reply for undesired chunk");
		return;
	}

	// One way or another, we're done with this chunk.
	wishlist -= ohash;
	if (current == ohash)
		current = QByteArray();
	else
		qDebug() << "ChunkPeer: received a chunk we weren't expecting";

	// The peer returns empty data if it can't supply the chunk requested.
	if (data.isEmpty()) {
		qDebug() << "Peer" << peerName()
			<< "failed to supply chunk" << ohash.toBase64();
		lost:
		req->status.insert(this, LostChunk);
		req->potentials.remove(this);
		return ChunkShare::checkRequest(req);
	}

	// Verify the chunk hash
	if (!(ohash == Sha512::hash(data))) {
		qDebug() << "Chunk received from" << peerName()
			<< "mismatches its hash:" << ohash.toBase64();
		goto lost;
	}

	qDebug() << "Received chunk" << ohash.toBase64()
		<< "from" << peerName();

	// Send the data to whoever's waiting for it
	// XX insert it in cache or tempfile too
	foreach (AbstractChunkReader *reader, req->readers) {
		if (reader == NULL) {
			qDebug() << "Reader disappeared while reading chunk";
			continue;
		}
		reader->gotData(ohash, data);
	}

	// This request is complete
	ChunkShare::requests.remove(ohash);
	delete req;

	// Look for other chunks to download
	ChunkShare::checkPeers();
}

