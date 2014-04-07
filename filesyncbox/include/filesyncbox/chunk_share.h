#pragma once

/**
 * Single-instance class that handles chunk sharing in the social network.
 */
class chunk_share : public peer_service, chunk_protocol
{
    /**
     * Outstanding chunk requests
     */
    static QHash<SST::PeerId, Request*> requests;

    /**
     * Maintain a persistent chunk query stream for each of our peers.
     */
    static QHash<SST::PeerId, chunk_peer*> peers;


    chunk_share();

public:
    static chunk_share *instance();

    static inline void init() { (void)instance(); }

    static void request_chunk(abstract_chunk_reader *reader,
                const byte_array &ohash);

private:
    static chunk_peer *peer(const SST::PeerId &hostid, bool create);

    /**
     * Check all peers for chunks they might be able to download.
     */
    static void check_peers();

    /**
     * Check a particular request to see if it can still be satisfied;
     * otherwise delete it and send noData() to all requestors.
     */
    static void check_request(Request *req);

// private slots:
    void got_out_stream_connected(Stream *stream);
    void got_out_stream_disconnected(Stream *stream);
    void got_in_stream_connected(Stream *stream);
};
