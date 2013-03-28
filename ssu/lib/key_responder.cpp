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
};

} // namespace negotiation
} // namespace ssu
