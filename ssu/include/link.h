#pragma once

#include <map>
#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
#include "byte_array.h"

namespace ssu {

class link;

typedef uint8_t channel_number;

/**
 * Add an association with particular link to the endpoint.
 */
class link_endpoint : public boost::asio::ip::udp::endpoint
{
    std::weak_ptr<link> link_;

    // Send a message to this endpoint on this socket
    bool send(const char *data, int size) const;
    inline bool send(const byte_array& msg) const {
        return send(msg.const_data(), msg.size());
    }
};

/**
 * Base class for socket-based channels,
 * for dispatching received packets based on endpoint and channel number.
 * May be used as an abstract base by overriding the receive() method,
 * or used as a concrete class by connecting to the received() signal.
 */
class link_channel
{
    std::weak_ptr<link> link_; ///< Link we're currently bound to, if any.
    bool active;               ///< True if we're sending and accepting packets.

    boost::signals2::signal<void (byte_array&, const link_endpoint&)> received;
    // Signalled when flow/congestion control may allow new transmission
    boost::signals2::signal<void ()> ready_transmit;
};

/**
 * Control protocol receiver.
 */
class link_receiver
{

};

/**
 * This class encapsulates link-related part of host state.
 */
class link_host_state
{
    virtual link* new_link();
};

/**
 * Abstract base class for entity connecting two endpoints using some network.
 * Link manages connection lifetime and maintains the connection status info.
 * For connected links there may be a number of channels established using their own keying schemes.
 * Link orchestrates initiation of key exchanges and scheme setup.
 */
class link
{
    link_host_state* const host;

    std::map<std::pair<link_endpoint, channel_number>, link_channel*> channels;

    bool active_;

public:
    link(link_host_state* h) : host(h) {}
};

/**
 * Class for UDP connection between two endpoints.
 * Multiplexes between channel-setup/key exchange traffic (which goes to key.cpp)
 * and per-channel data traffic (which goes to channel.cpp).
 */
class udp_link : public link
{
    boost::asio::ip::udp::socket udp_socket;
};

} // namespace ssu
