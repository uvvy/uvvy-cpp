//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "abstract_stream.h"

namespace ssu {

/**
 * @internal
 * Basic internal implementation of the abstract stream.
 * The separation between the internal stream control object and the application-visible stream
 * object is primarily needed so that ssu can hold onto a stream's state and gracefully shut it down
 * after the application deletes its stream object representing it.
 * This separation also keeps the internal stream control variables out of the public C++ API
 * header files and thus able to change without breaking binary compatibility, and makes it easy
 * to implement service/protocol negotiation for top-level application streams by 
 * extending base_stream.
 */
class base_stream : public abstract_stream
{
    enum class state {
        fresh = 0,     ///< Newly created.
        wait_service,  ///< Initiating, waiting for service reply.
        accepting,     ///< Accepting, waiting for service request.
        connected,     ///< Connection established.
        disconnected   ///< Connection terminated.
    };

    /**
     * @internal
     * Unit of data transmission on ssu stream.
     */
    struct packet
    {
        base_stream* owner;
        uint64_t tsn;     ///< Logical byte position. XXX tx_byte_pos
        byte_array buf;   ///< Packet buffer including headers.
        int hdrlen;       ///< Size of channel and stream headers.
        packet_type type; ///< Type of this packet.
        bool late;        ///< Possibly lost packet.

        inline packet()
            : owner(nullptr)
            , type(packet_type::invalid)
        {}
        inline packet(base_stream* o, packet_type t)
            : owner(o)
            , type(t)
        {}
        inline bool is_null() const {
            return owner == nullptr;
        }
        inline int payload_size() const {
            return buf.size() - hdrlen;
        }
    };

    std::weak_ptr<base_stream> parent; ///< Parent, if it still exists.

    static const size_t default_rx_buffer_size = 65536;

public:
};

} // namespace ssu
