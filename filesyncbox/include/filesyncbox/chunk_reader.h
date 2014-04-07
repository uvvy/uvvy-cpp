#pragma once

#include "filesyncbox/abstract_chunk_reader.h"

class chunk_reader : public abstract_chunk_reader
{
    inline chunk_reader() : abstract_chunk_reader() {}

// signals:
    void got_data(byte_array const& outer_hash, byte_array const& data) override;
    void no_data(byte_array const& outer_hash) override;
};
