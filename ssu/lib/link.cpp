//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "link.h"
#include "logging.h"

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

	byte_array_buf buf(msg);
	std::istream in(&buf);
	boost::archive::binary_iarchive ia(in, boost::archive::no_header);
	magic_t magic;
	ia >> magic;
	link_receiver* recv = host.receiver(magic);
	if (recv)
	{
		return recv->receive(msg, ia, src);
	}

	debug() << "Received an invalid message, ignoring unknown channel/receiver " << std::hex << std::showbase << magic << " buffer contents " << msg;
}

udp_link::udp_link(boost::asio::io_service& io_service, const endpoint& ep, link_host_state& h)
	: link(h)
	, udp_socket(io_service, ep)
{
	// received_buffer.resize(256);
	prepare_async_receive();
}

void udp_link::prepare_async_receive()
{
	udp_socket.async_receive_from(
		boost::asio::buffer(received_buffer.data(), received_buffer.size()),
		received_from,
		boost::bind(&udp_link::udp_ready_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
}

// bool udp_link::bind(const endpoint& ep)
// {
// 	// once bound, can start receiving datagrams.
// 	prepare_async_receive();
// 	return true;
// }

bool udp_link::send(const endpoint& ep, const char *data, int size)
{
	return udp_socket.send_to(boost::asio::buffer(data, size), ep);
}

void udp_link::udp_ready_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	receive(received_buffer, received_from);
	// processed received buffers, continue receiving datagrams.
	prepare_async_receive();
}

}
