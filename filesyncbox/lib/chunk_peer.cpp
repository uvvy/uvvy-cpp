namespace ssu {
namespace internal {

chunk_peer::chunk_peer(const PeerId &eid)
    : eid_(eid)
{
}

chunk_peer::~chunk_peer()
{
    // Remove any references to us from all outstanding chunk requests
    remove_from_requests();
}

QString chunk_peer::peer_name()
{
    return chunk_share::instance()->peerTable()->name(peerHostId());
}

void chunk_peer::remove_from_requests()
{
    foreach (Request *req, chunk_share::requests) {
        req->potentials.remove(this);
        req->status.remove(this);
        chunk_share::checkRequest(req);
    }
}

void chunk_peer::send_status_request(Request *req)
{
    Stream *stream = chunk_share::instance()->connectToPeer(eid_);
    Q_ASSERT(stream);
    if (!stream->isConnected()) {
        qDebug() << "Peer" << peerName() << "is unreachable for chunk status request";
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

void chunk_peer::check_work()
{
    Stream *stream = chunk_share::instance()->connectToPeer(eid_);
    if (!stream->isConnected())
        return;     // Unconnected peers can't do work
    if (!current.isEmpty())
        return;     // Already busy downloading a chunk
    if (wishlist.isEmpty())
        return;     // Nothing to do

    // Find a chunk on this peer's wishlist
    // that no one else is downloading at the moment.
    QByteArray selhash;
    foreach (const QByteArray &ohash, wishlist) {

        // Make sure this chunk is still actually needed.
        if (chunk_share::requests.value(ohash) == NULL) {
            wishlist.remove(ohash);
            continue;
        }

        // Make sure no one else is already downloading it.
        bool found = false;
        foreach (chunk_peer *p2, chunk_share::peers) {
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
        qDebug() << "Nothing non-redundant for peer" << peerName() << "to do.";
        return;
    }

    // Request the selected chunk from this peer
    qDebug() << "Requesting chunk" << selhash.toBase64() << "from peer" << peerName();
    QByteArray msg;
    XdrStream ws(&msg, QIODevice::WriteOnly);
    ws << (qint32)ChunkRequest << selhash;
    stream->writeMessage(msg);

    // Remember that we're waiting for this chunk
    current = selhash;
}

void chunk_peer::peer_read_message()
{
    Stream *strm = (Stream*)sender();
    Q_ASSERT(strm);

    forever {
        if (!strm->isLinkUp() || strm->atEnd()) {
            qDebug() << "chunk_peer:" << peerName() << "disconnected";
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
            qDebug() << "chunk_share: unknown message code" << msgcode;
            break;
        }
    }
}

void chunk_peer::got_status_request(Stream *stream, XdrStream &rs)
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

void chunk_peer::got_chunk_request(Stream *stream, XdrStream &rs)
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

void chunk_peer::got_status_reply(Stream *, XdrStream &rs)
{
    QByteArray ohash;
    qint32 st;
    rs >> ohash >> st;
    if (rs.status() != rs.Ok || ohash.isEmpty()) {
        qDebug("Invalid chunk status reply message");
        return;
    }

    Request *req = chunk_share::requests.value(ohash);
    if (!req) {
        qDebug("Got useless chunk status reply for undesired chunk");
        return;
    }

    req->status.insert(this, (ChunkStatus)st);
    switch ((ChunkStatus)st) {
    case OnlineChunk:
        wishlist.insert(ohash);
        chunk_share::checkPeers();
        break;
    default:
        qDebug() << "Got unknown chunk status" << st;
        // fall through...
    case UnknownChunk:
        req->potentials.remove(this);
        chunk_share::checkRequest(req);
        break;
    }
}

void chunk_peer::got_chunk_reply(Stream *, XdrStream &rs)
{
    QByteArray ohash, data;
    rs >> ohash >> data;
    if (rs.status() != rs.Ok || ohash.isEmpty()) {
        qDebug("Invalid chunk status reply message");
        return;
    }

    Request *req = chunk_share::requests.value(ohash);
    if (!req) {
        qDebug("Got useless chunk reply for undesired chunk");
        return;
    }

    // One way or another, we're done with this chunk.
    wishlist -= ohash;
    if (current == ohash)
        current = QByteArray();
    else
        qDebug() << "chunk_peer: received a chunk we weren't expecting";

    // The peer returns empty data if it can't supply the chunk requested.
    if (data.isEmpty()) {
        qDebug() << "Peer" << peerName() << "failed to supply chunk" << ohash.toBase64();
        lost:
        req->status.insert(this, LostChunk);
        req->potentials.remove(this);
        return chunk_share::checkRequest(req);
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
    chunk_share::requests.remove(ohash);
    delete req;

    // Look for other chunks to download
    chunk_share::checkPeers();
}

} // internal namespace
} // ssu namespace
