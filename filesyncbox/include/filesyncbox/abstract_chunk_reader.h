#pragma once

#include <boost/signals2/signal.hpp>
#include "arsenal/byte_array.h"

/**
 * Base interface for chunk data provider.
 * It supports only one operation: request chunk of data by its outer hash.
 * It can respond in one of two ways: by returning the data via on_got_data signal
 * or by giving up via on_no_data signal. You could also override virtual got_data() and
 * no_data() in your subclass to provide additional handling.
 */
class abstract_chunk_reader
{
protected:
    inline abstract_chunk_reader() {}

    /**
     * Submit a request to read a chunk.
     * For each such chunk request this class will send either
     * a gotData() or a noData() signal for the requested chunk
     * (unless the ChunkReader gets destroyed first).
     * The caller can use a single ChunkReader object
     * to read multiple chunks simultaneously.
     */
    void read_chunk(byte_array const& ohash);

    boost::signals2::signal<void (byte_array const&, byte_array const&)> on_got_data;
    boost::signals2::signal<void (byte_array const&)> on_no_data;

    /**
     * The chunk reader calls these methods when a chunk arrives,
     * or when it gives up on finding a chunk.
     */
    virtual void got_data(byte_array const& outer_hash, byte_array const& data) {
        on_got_data(outer_hash, data);
    }
    virtual void no_data(byte_array const& outer_hash) {
        on_no_data(outer_hash);
    }
};
