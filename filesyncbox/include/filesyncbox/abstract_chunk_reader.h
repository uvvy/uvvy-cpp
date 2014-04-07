#pragma once

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
    void read_chunk(const QByteArray &ohash);

    /**
     * The chunk reader calls these methods when a chunk arrives,
     * or when it gives up on finding a chunk.
     */
    virtual void got_data(const byte_array &outer_hash, const byte_array &data) = 0;
    virtual void no_data(const byte_array &outer_hash) = 0;
};
