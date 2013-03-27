//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/endian/conversion2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include "link.h"
#include "logging.h"

namespace ssu {

bool link_endpoint::send(const char *data, int size) const
{
    if (auto l = link_.lock())
    {
        return l->send(*this, data, size);
    }
    logger::debug() << "Trying to send on a nonexistent socket";
    return false;
}

link::~link()
{

}

/**
 * Two packet types we could receive are stream packet (multiple types), starting with channel header
 * with non-zero channel number. It is handled by specific link_channel.
 * Another type is negotiation packet which usually attempts to start a session negotiation, it should
 * have zero channel number. It is handled by registered link_receiver (XXX rename to link_responder?).
 *
 *  Channel header (8 bytes)
 *   31          24 23                                0
 *  +--------------+-----------------------------------+
 *  |   Channel    |     Transmit Seq Number (TSN)     |
 *  +------+-------+-----------------------------------+
 *  | Rsvd | AckCt | Acknowledgement Seq Number (ASN)  |
 *  +------+-------+-----------------------------------+
 *        ... more channel-specific data here ...
 *
 *  Negotiation header (8+ bytes)
 *   31          24 23                                0
 *  +--------------+-----------------------------------+
 *  |  Channel=0   |     Negotiation Magic Bytes       |
 *  +--------------+-----------------------------------+
 *  |                  Chunk #1 size                   |
 *  +--------------------------------------------------+
 *  |                     .....                        |
 *  |                    Chunk #1                      |
 *  |                     .....                        |
 *  +--------------------------------------------------+
 *
 */
void link::receive(byte_array& msg, const link_endpoint& src)
{
    if (msg.size() < 4)
    {
        logger::debug() << "Ignoring too small UDP datagram";
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

    boost::iostreams::filtering_istream in(boost::make_iterator_range(msg.as_vector()));
    boost::archive::binary_iarchive ia(in, boost::archive::no_header);
    magic_t magic;
    ia >> magic;
    magic = boost::endian2::big(magic);
    link_receiver* recv = host.receiver(magic);
    if (recv)
    {
        return recv->receive(msg, ia, src);
    }

    logger::debug() << "Received an invalid message, ignoring unknown channel/receiver " << hex(magic, 8, true) 
                    << " buffer contents " << msg;
}

udp_link::udp_link(boost::asio::io_service& io_service, const endpoint& ep, link_host_state& h)
    : link(h)
    , udp_socket(io_service, ep)
{
    prepare_async_receive();
}

void udp_link::prepare_async_receive()
{
    boost::asio::streambuf::mutable_buffers_type buffer = received_buffer.prepare(2048);
    udp_socket.async_receive_from(
        boost::asio::buffer(buffer),
        received_from,
        boost::bind(&udp_link::udp_ready_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
}

// bool udp_link::bind(const endpoint& ep)
// {
//  // once bound, can start receiving datagrams.
//  prepare_async_receive();
//  return true;
// }

bool udp_link::send(const endpoint& ep, const char *data, int size)
{
    return udp_socket.send_to(boost::asio::buffer(data, size), ep);
}

void udp_link::udp_ready_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error)
    {
        logger::debug() << "Received " << bytes_transferred << " bytes via UDP link";
        byte_array b(boost::asio::buffer_cast<const char*>(received_buffer.data()), bytes_transferred);
        receive(b, received_from);
        received_buffer.consume(bytes_transferred);
        // processed received buffers, continue receiving datagrams. @todo: reverse, allowing to receive datagrams before the previous was processed?
        prepare_async_receive();
    }
}

}
