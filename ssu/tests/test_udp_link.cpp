//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_TEST_MODULE Test_ssu_link
#include <boost/test/unit_test.hpp>

#include "link.h"
#include "negotiation/key_message.h"
#include "negotiation/key_responder.h"

BOOST_AUTO_TEST_CASE(receive_and_log_key_message)
{
	ssu::link_host_state host;
    ssu::endpoint local_ep(boost::asio::ip::udp::v4(), 9660);
    ssu::udp_link l(local_ep, host);

    // Add key responder to link.
    ssu::negotiation::key_responder receiver;
    host.bind_receiver(ssu::stream_protocol::magic, &receiver);

    // Render key message to buffer.
    byte_array msg;

    {
        boost::iostreams::filtering_ostream out(boost::iostreams::back_inserter(msg.as_vector()));
        boost::archive::binary_oarchive oa(out, boost::archive::no_header);
        ssu::negotiation::key_message m;
        ssu::negotiation::key_chunk k;
        ssu::negotiation::dh_init1_chunk dh;

        dh.group = ssu::negotiation::dh_group_type::dh_group_1024;
        dh.key_min_length = 0x10;

        dh.initiator_hashed_nonce.resize(32);
        for (int i = 0; i < 32; ++i)
            dh.initiator_hashed_nonce[i] = rand();
        dh.initiator_dh_public_key.resize(128);
        for (int i = 0; i < 128; ++i)
            dh.initiator_dh_public_key[i] = 255 - i;

        k.type = ssu::negotiation::key_chunk_type::dh_init1;
        k.dh_init1 = dh;

        m.magic = ssu::stream_protocol::magic;
        m.chunks.push_back(k);

        oa << m;
    }

    // and send it to ourselves.
    l.send(local_ep, msg.data(), msg.size());//hmmm, send(ep, byte_array) doesn't work, why??

    host.run_io_service();
}
