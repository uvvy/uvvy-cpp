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
#include "crypto.h"

// Supplemental functions.
namespace {

// Calculate SHA256 hash of the key response message.
byte_array calc_signature_hash(ssu::negotiation::dh_group_type group, int keylen,
    const byte_array& initiator_hashed_nonce,
    const byte_array& responder_nonce,
    const byte_array& initiator_dh_public_key,
    const byte_array& responder_dh_public_key,
    const byte_array& peer_eid)
{
    byte_array data;
    {
        boost::iostreams::filtering_ostream out(boost::iostreams::back_inserter(data.as_vector()));
        boost::archive::binary_oarchive oa(out, boost::archive::no_header); // Key parameter signing block for init2 and response2 messages.

        oa << group;// DH group for public keys
        oa << keylen;// AES key length: 16, 24, or 32
        oa << initiator_hashed_nonce;
        oa << responder_nonce;
        oa << initiator_dh_public_key;
        oa << responder_dh_public_key;
        oa << peer_eid;
        // @todo the byte arrays should be 4-byte boundary zero-padded
    }

    crypto::hash md;
    crypto::hash::value hash;
    md.update(data.as_vector());
    md.finalize(hash);

    return hash;
}

}

namespace ssu {
namespace negotiation {

void key_responder::receive(const byte_array& msg, const link_endpoint& src)
{
    boost::iostreams::filtering_istream in(boost::make_iterator_range(msg.as_vector()));
    boost::archive::binary_iarchive ia(in, boost::archive::no_header);
	key_message m;
	ia >> m;
    // XXX here may be some decoding error...

    assert(m.magic == stream_protocol::magic);

    for (auto chunk : m.chunks)
    {
        switch (chunk.type)
        {
            case ssu::negotiation::key_chunk_type::dh_init1:
                return got_dh_init1(*chunk.dh_init1, src);
            case ssu::negotiation::key_chunk_type::dh_init2:
                return got_dh_init2(*chunk.dh_init2, src);
            default:
                logger::warning() << "Unknown negotiation chunk type " << uint32_t(chunk.type);
                break;
        }
    }
};

void key_responder::got_dh_init1(const dh_init1_chunk& data, const link_endpoint& src)
{

}

void key_responder::got_dh_init2(const dh_init2_chunk& data, const link_endpoint& src)
{
    ssu::negotiation::dh_group_type group = ssu::negotiation::dh_group_type::dh_group_1024;
    int keylen = 16;
    byte_array initiator_hashed_nonce;
    byte_array responder_nonce;
    byte_array initiator_dh_public_key;
    byte_array responder_dh_public_key;
    byte_array peer_eid;
    byte_array hash = calc_signature_hash(group, keylen, initiator_hashed_nonce, responder_nonce, initiator_dh_public_key, responder_dh_public_key, peer_eid);
}

} // namespace negotiation
} // namespace ssu
