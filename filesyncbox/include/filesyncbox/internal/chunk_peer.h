#pragma once

#include "filesyncbox/chunk_protocol.h"

namespace ssu {
namespace internal {

/**
 * Private helper class for chunk_share:
 * implements our interaction with a given peer for chunk sharing.
 */
class chunk_peer : public chunk_protocol
{
    /**
     * Minimum delay between successive connection attempts - 1 minute
     */
    static const int recon_delay = 60*1000;

    peer_id const eid_;             ///< Host ID of peer
    std::set<byte_array> wishlist;  ///< What we want from this host
    byte_array current;             ///< Ohash of chunk being gotten

public:
    chunk_peer(peer_id const& hostid);
    ~chunk_peer();

    inline peer_id peer_host_id() { return hostid; }
    string peer_name();

    void check_work();

    void send_status_request(request *req);

    void got_status_request(stream *strm, byte_array_iwrap<flurry::iarchive>& is);
    void got_chunk_request(stream *strm, byte_array_iwrap<flurry::iarchive>& is);

    void got_status_reply(stream *strm, byte_array_iwrap<flurry::iarchive>& is);
    void got_chunk_reply(stream *strm, byte_array_iwrap<flurry::iarchive>& is);

    void remove_from_requests();

    void peer_read_message();
};

} // internal namespace
} // ssu namespace
