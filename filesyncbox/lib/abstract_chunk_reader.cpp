//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "filesyncbox/abstract_chunk_reader.h"

void abstract_chunk_reader::read_chunk(byte_array const& ohash)
{
    byte_array data = store::read_stores(ohash);
    if (!data.is_empty())
        return got_data(ohash, data);

    // Request asynchronously from other nodes
    chunk_share::request_chunk(this, ohash);
}

