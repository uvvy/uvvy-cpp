#include "chunk_share.h"

std::map<peer_id, chunk_share::request*> chunk_share::requests;
std::map<peer_id, chunk_peer*> chunk_share::peers;

chunk_share::chunk_share()
    : peer_service("metta:ChunkShare", "Chunked data sharing",
                   "DataShare", "MettaNode data sharing protocol")
{
    on_out_stream_connected.connect([this](stream* s) {
        got_out_stream_connected(s);
    });
    on_out_stream_disconnected.connect([this](stream* s) {
        got_out_stream_disconnected(s);
    });
    on_in_stream_connected.connect([this](stream* s) {
        got_in_stream_connected(s);
    });
}

chunk_share*
chunk_share::instance()
{
    static chunk_share obj;
    return &obj;
}

chunk_peer*
chunk_share::peer(peer_id const& hostid, bool create)
{
    if (!create) {
        return peers.value(hostid);
    }

    chunk_peer *&peer = peers[hostid];
    if (!peer && create) {
        peer = new chunk_peer(hostid);
    }
    return peer;
}

void
chunk_share::request_chunk(abstract_chunk_reader *reader, byte_array const& ohash)
{
    qDebug() << "chunk_share: searching for chunk" << ohash.toBase64();

    // Make sure we're set up
    init();

    // Find or create a Request for this chunk
    Request *&req = requests[ohash];
    if (!req)
        req = new Request(ohash);
    req->readers.insert(reader);

    // Send a chunk status request to each of our current peers
    foreach (chunk_peer *peer, peers)
        peer->send_status_request(req);
}

void chunk_share::check_request(request *req)
{
    Q_ASSERT(req);

    if (!req->potentials.isEmpty())
        return;     // Still have places to look

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
        reader->no_data(req->ohash);
    }

    // Delete the request
    delete req;
}

void chunk_share::check_peers()
{
    // Make sure each peer has something useful to do
    foreach (chunk_peer *peer, peers)
        peer->checkWork();
}

void chunk_share::got_out_stream_connected(stream *stream)
{
    // Find or create the appropriate chunk_peer
    chunk_peer *peer = peer(stream->remote_host_id(), true);

    // Watch for messages on this stream
    stream->on_ready_read_message.connect([peer, stream]() {
        peer->peer_read_message(stream);
    });

    // Reset current chunk downloading state for this peer,
    // since any chunk we were downloading probably got lost
    // when our last outgoing connection (if any) failed.
    peer->current = byte_array();

    // Unconditionally re-send all outstanding status requests.
    for (request *req : chunk_share::requests) {
        peer->send_status_request(req);
    }
}

void chunk_share::got_out_stream_disconnected(Stream *stream)
{
    chunk_peer *peer = this->peer(stream->remoteHostId(), false);
    if (!peer)
        return;

    // If the primary stream has been disconnected,
    // remove this peer from all outstanding requests;
    // it obviously won't be able to do much for us at the moment.
    peer->remove_from_requests();
}

void chunk_share::got_in_stream_connected(Stream *stream)
{
    // Find or create the appropriate chunk_peer
    chunk_peer *peer = this->peer(stream->remoteHostId(), true);

    // Watch for messages on this stream
    connect(stream, SIGNAL(readyReadMessage()),
        peer, SLOT(peerReadMessage()));
}

