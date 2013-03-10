#pragma once

typedef uint8_t channel_number;

/**
 * Base class for socket-based channels.
 */
class link_channel
{

};

/**
 * Control protocol receiver.
 */
class link_receiver
{

};

/**
 * Abstract base class for entity connecting two endpoints using some network.
 */
class link
{
	link_host_state* const host;

	map<pair<endpoint, channel_number>, link_channel*> channels;

	bool active_;
};

/**
 * Add an association with particular link to the endpoint.
 */
class link_endpoint : public endpoint
{};

/**
 * Class for UDP connection between two endpoints.
 * Multiplexes between channel-setup/key exchange traffic (which goes to key.cpp)
 * and per-channel data traffic (which goes to channel.cpp).
 */
class udp_link : public link
{
	boost::asio::ip::udp::socket udp_socket;
};

class link_host_state
{
	virtual link* new_link();
};

