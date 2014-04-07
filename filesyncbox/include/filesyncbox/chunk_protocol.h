//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "arsenal/byte_array.h"

namespace ssu {

/**
 * Base pseudo-class to provide symbols shared by several chunk classes.
 */
class chunk_protocol
{
public:
    /// Chunk message codes
    enum msg_code : uint32_t
    {
        chunk_status_request  = 0x101,
        chunk_request         = 0x102,

        chunk_status_reply    = 0x201,
        chunk_reply           = 0x202,
    };

    // Chunk status codes
    enum chunk_status : uint32_t
    {
        invalid_status = 0,
        online_chunk,        // Have it immediately available
        offline_chunk,       // Might be available with user effort
        unknown_chunk,       // Don't know anything about it
        lost_chunk,          // Used to have it, but lost
    };

    struct request
    {
        byte_array const outer_hash; // Chunk identity

        set<abstract_chunk_reader*> readers;  // who wants it

        // Potential peers that might have this chunk.
        // A peer is included if we've sent it a status request,
        // or if it has positively reported having the chunk.
        set<chunk_peer*> potentials;

        // Who knows what about this chunk.
        // We use the 'InvalidStatus' code here to mean
        // we've requested status from this peer but not received it.
        QHash<chunk_peer*, chunk_status> status;


        inline request(byte_array const& ohash) : outer_hash(ohash) {}
    };
};

} // ssu namespace
