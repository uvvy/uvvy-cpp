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
    {
        std::ofstream os("testdata.bin", std::ios_base::binary|std::ios_base::out|std::ios::trunc);
        boost::archive::binary_oarchive oa(os, boost::archive::no_header);

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
        std::ifstream is("testdata.bin", std::ios_base::binary|std::ios_base::in);
        boost::archive::binary_iarchive ia(is, boost::archive::no_header);
        ssu::negotiation::key_message m;

        ia >> m;

        BOOST_CHECK(m.magic == stream_protocol::magic);
        BOOST_CHECK(m.chunks.size() == 1);
        BOOST_CHECK(m.chunks[0].type == ssu::negotiation::key_chunk_type::dh_init1);
        BOOST_CHECK(m.chunks[0].dh_init1.is_initialized());
        BOOST_CHECK(m.chunks[0].dh_init1->group == ssu::negotiation::dh_group_type::dh_group_1024);
        BOOST_CHECK(m.chunks[0].dh_init1->key_min_length = 0x10);
        BOOST_CHECK(m.chunks[0].dh_init1->initiator_hashed_nonce.size() == 32);
        for (int i = 0; i < 128; ++i) {
            assert(m.chunks[0].dh_init1->initiator_dh_public_key[i] == 255 - i);
        }
        m.chunks[0].dh_init1->dump();
    }
}
