#pragma once

#include "abstract_stream.h"

namespace ssu {

/**
 * @internal
 * Basic internal implementation of the abstract stream.
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
            , type(invalid_packet)
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

    weak_ptr<base_stream> parent; ///< Parent, if it still exists.

public:
};

} // namespace ssu
