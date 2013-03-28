//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/optional/optional.hpp>
#include "protocol.h"
#include "byte_array.h"
#include "custom_optional.h"
#include "xdr.h"
#include "negotiation/key_message.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test_key_message_serialization
#include <boost/test/unit_test.hpp>

#include "link.h"

using namespace std;

/**
 * Generate a binary blob for testing key_message.h i/o functions.
 * Create a key message with dh_init1 and checksum_init chunks, add packet chunk of data afterwards.
 *
 * +-------+--------+-----------------------------------------------------------------+
 * | magic | count  | opaque                                                          |
 * +-------+--------+--------+-------+------------------------------------------------+
 *                  | length | discr |                                                |
 *                  +--------+-------+------------------------------------------------+
 *                     0xb4    0x21  | DHgroup | keymin | nhi | length | dhi | length |
 *                                   +---------+--------+-----+--------+-----+--------+
 *                                       0x1      0x10    32b
 */
BOOST_AUTO_TEST_CASE(serialize_and_deserialize)
{
    byte_array data;
    {
        boost::iostreams::filtering_ostream out(boost::iostreams::back_inserter(data.as_vector()));
        boost::archive::binary_oarchive oa(out, boost::archive::no_header);

        ssu::negotiation::key_message m;
        ssu::negotiation::key_chunk chu;
        ssu::negotiation::dh_init1_chunk dh;

        m.magic = stream_protocol::magic;
        chu.type = ssu::negotiation::key_chunk_type::dh_init1;
        dh.group = ssu::negotiation::dh_group_type::dh_group_1024;
        dh.key_min_length = 0x10;

        dh.initiator_hashed_nonce.resize(32);
        for (int i = 0; i < 32; ++i)
            dh.initiator_hashed_nonce[i] = rand();
        dh.initiator_dh_public_key.resize(128);
        for (int i = 0; i < 128; ++i)
            dh.initiator_dh_public_key[i] = 255 - i;

        chu.dh_init1 = dh;

        m.chunks.push_back(chu);
        oa << m;
    }
    {
        boost::iostreams::filtering_istream in(boost::make_iterator_range(data.as_vector()));
        boost::archive::binary_iarchive ia(in, boost::archive::no_header);
        ssu::negotiation::key_message m;

        BOOST_CHECK(data.size() == 192);

        ia >> m;

        BOOST_CHECK(m.magic == stream_protocol::magic);
        BOOST_CHECK(m.chunks.size() == 1);
        BOOST_CHECK(m.chunks[0].type == ssu::negotiation::key_chunk_type::dh_init1);
        BOOST_CHECK(m.chunks[0].dh_init1.is_initialized());
        BOOST_CHECK(m.chunks[0].dh_init1->group == ssu::negotiation::dh_group_type::dh_group_1024);
        BOOST_CHECK(m.chunks[0].dh_init1->key_min_length = 0x10);
        BOOST_CHECK(m.chunks[0].dh_init1->initiator_hashed_nonce.size() == 32);
        for (int i = 0; i < 128; ++i) {
            BOOST_CHECK(uint8_t(m.chunks[0].dh_init1->initiator_dh_public_key[i]) == 255 - i);
        }
    }
}
