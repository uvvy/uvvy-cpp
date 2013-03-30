//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "negotiation/key_responder.h"
#include "negotiation/key_message.h"

namespace ssu {
namespace negotiation {

void key_responder::receive(const byte_array& msg, const link_endpoint& src)
{
    boost::iostreams::filtering_istream in(boost::make_iterator_range(msg.as_vector()));
    boost::archive::binary_iarchive ia(in, boost::archive::no_header);
	key_message m;
	ia >> m;
    // XXX here may be some decoding error...

    // for now only expect one type of handshake chunk - DH init1
    assert(m.magic == stream_protocol::magic);
    assert(m.chunks[0].type == ssu::negotiation::key_chunk_type::dh_init1);
    got_dh_init1(*m.chunks[0].dh_init1, src);
};

void key_responder::got_dh_init1(const dh_init1_chunk& data, const link_endpoint& src)
{

}

} // namespace negotiation
} // namespace ssu
