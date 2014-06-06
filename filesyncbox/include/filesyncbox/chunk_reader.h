//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "filesyncbox/abstract_chunk_reader.h"

class chunk_reader : public abstract_chunk_reader
{
    inline chunk_reader() : abstract_chunk_reader() {}

// signals:
    void got_data(byte_array const& outer_hash, byte_array const& data) override;
    void no_data(byte_array const& outer_hash) override;
};
