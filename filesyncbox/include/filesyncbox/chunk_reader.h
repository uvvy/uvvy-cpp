#pragma once

#include "filesyncbox/abstract_chunk_reader.h"

class chunk_reader : public abstract_chunk_reader
{
    inline chunk_reader() : abstract_chunk_reader() {}

// signals:
    void got_data(const byte_array &outer_hash, const byte_array &data) override;
    void no_data(const byte_array &outer_hash) override;
};
