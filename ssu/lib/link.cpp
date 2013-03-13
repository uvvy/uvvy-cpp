#include "link.h"

namespace ssu {

bool link_endpoint::send(const char *data, int size) const
{
	if (auto l = link_.lock())
	{
		return l->send(*this, data, size);
	}
	debug() << "Trying to send on a nonexistent socket";
	return false;
}

link::~link()
{

}

void link::receive(byte_array& msg, const link_endpoint& src)
{
	if (msg.size() < 4)
	{
		debug() << "Ignoring too small UDP datagram";
		return;
	}

	// First byte should be a channel number.
	// Try to find an endpoint-specific channel.
	channel_number cn = msg.at(0);
	link_channel* chan = channel(src, cn);
	if (chan)
	{
		return chan->receive(msg, src);
	}

	ixdrstream in(&msg);
	magic_t magic;
	in >> magic;
	link_receiver* recv = host->receiver(magic);
	if (recv)
	{
		return recv->receive(msg, in, src);
	}

	debug() << "Received an invalid message, ignoring unknown channel/receiver" << std::hex << magic << msg;
}

bool udp_link::bind(const endpoint& ep)
{
	// once bound, can start receiving datagrams.
	udp_socket.async_receive_from(boost::asio::buffer(receive_buffer), received_from, boost::bind(udp_link::udp_ready_read, this));
}

bool udp_link::send(const endpoint& ep, const char *data, int size)
{
	udp_socket.send_to(boost::asio::buffer(data, size), ep);
}

void udp_link::udp_ready_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	receive(receive_buffer, received_from);
	// processed received buffers, continue receiving datagrams.
	udp_socket.async_receive_from(boost::asio::buffer(receive_buffers), received_from, boost::bind(udp_link::udp_ready_read, this));
}

}
