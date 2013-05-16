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
#include "host.h"

// some temporary stuff to get things compile
// Length of symmetric key material for HMAC-SHA-256-128
#define HMACKEYLEN  (256/8)
// We use SHA-256 hashes truncated to 128 bits for HMAC generation
#define HMACLEN     (128/8)
//end temp junk

//===========================================================================================================
// Supplemental functions.
//===========================================================================================================

namespace {

/**
 * Calculate SHA-256 hash of the key response message.
 */
byte_array
calc_signature_hash(ssu::negotiation::dh_group_type group,
    int keylen,
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
        xdr::pad_size(oa, initiator_hashed_nonce.size());
        oa << responder_nonce;
        xdr::pad_size(oa, responder_nonce.size());
        oa << initiator_dh_public_key;
        xdr::pad_size(oa, initiator_dh_public_key.size());
        oa << responder_dh_public_key;
        xdr::pad_size(oa, responder_dh_public_key.size());
        oa << peer_eid;
        xdr::pad_size(oa, peer_eid.size());
    }
    assert(data.size() % 4 == 0);
    logger::file_dump dump(data);

    crypto::hash md;
    crypto::hash::value sha256hash;
    md.update(data.as_vector());
    md.finalize(sha256hash);

    return sha256hash;
}

/**
 * Compute HMAC challenge cookie for DH.
 */
byte_array
calc_dh_cookie(ssu::negotiation::dh_hostkey_t* hostkey,
    const byte_array& responder_nonce,
    const byte_array& initiator_hashed_nonce,
    const ssu::link_endpoint& src)
{
    byte_array data;
    {
        boost::iostreams::filtering_ostream out(boost::iostreams::back_inserter(data.as_vector()));
        boost::archive::binary_oarchive oa(out, boost::archive::no_header); // Put together the data to hash

        oa << hostkey->public_key;
        xdr::pad_size(oa, hostkey->public_key.size());
        oa << responder_nonce;
        xdr::pad_size(oa, responder_nonce.size());
        oa << initiator_hashed_nonce;
        xdr::pad_size(oa, initiator_hashed_nonce.size());
        // @todo XDR zero-padding...
        auto lval_addr = src.address().to_v4().to_bytes();
        oa << lval_addr;
        uint16_t port = src.port();
        oa << port;
    }

    logger::file_dump dump(data);

    // Compute the keyed hash
    assert(hostkey->hkr.size() == HMACKEYLEN);

    crypto::hash kmd(hostkey->hkr.as_vector());
    crypto::hash::value mac;
    // assert(mac.size() == HMACLEN);//mmmhm, expected HMACLEN is 16 but we generate 32 bytes HMACs... incompat?
    kmd.update(data.as_vector());
    kmd.finalize(mac);

    return mac;
}

}

//===========================================================================================================
// key_responder
//===========================================================================================================

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
            case ssu::negotiation::key_chunk_type::dh_response1:
                return got_dh_response1(*chunk.dh_response1, src);//key_initiator method?
            default:
                logger::warning() << "Unknown negotiation chunk type " << uint32_t(chunk.type);
                break;
        }
    }
};

static void warning(std::string message)
{
    logger::warning() << "key_responder: " << message;
}

/**
 * Send complete prepared key_message.
 */
static void send(key_message& m, const link_endpoint& target)
{
    byte_array msg;
    {
        boost::iostreams::filtering_ostream out(boost::iostreams::back_inserter(msg.as_vector()));
        boost::archive::binary_oarchive oa(out, boost::archive::no_header);
        oa << m;
    }
    target.send(msg);
}

static void send(magic_t magic, dh_init1_chunk& r, const link_endpoint& to)
{
    key_message m;
    key_chunk chunk;

    chunk.type = key_chunk_type::dh_init1;
    chunk.dh_init1 = r;

    m.magic = magic;
    m.chunks.push_back(chunk);

    send(m, to);
}

static void send(magic_t magic, dh_response1_chunk& r, const link_endpoint& to)
{
    key_message m;
    key_chunk chunk;

    chunk.type = key_chunk_type::dh_response1;
    chunk.dh_response1 = r;

    m.magic = magic;
    m.chunks.push_back(chunk);

    send(m, to);
}

void key_responder::got_dh_init1(const dh_init1_chunk& data, const link_endpoint& src)
{
    logger::debug() << "Got dh_init1";

    if (data.key_min_length != 128/8 and data.key_min_length != 192/8 and data.key_min_length != 256/8)
        return warning("invalid minimum AES key length");

    dh_hostkey_t* hostkey = host_.lock()->get_dh_key(data.group); // get or generate a host key
    // Problem: once we've got the hostkey here, the timeout timer may fire and cause host's dh key to expire.
    // @todo Need to protect against this race here... (e.g. return hostkey in shared_ptr)
    if (!hostkey)
        return warning("unrecognized DH key group");
    // if (i1.dhi.size() > DH_size(hk->dh))
        // return;     // Public key too large

    // Generate an unpredictable responder's nonce
    assert(crypto::prng_ok());
    boost::array<uint8_t, crypto::hash::size> responder_nonce;
    crypto::fill_random(responder_nonce);

    // Compute the hash challenge
    byte_array challenge_cookie = calc_dh_cookie(hostkey, responder_nonce, data.initiator_hashed_nonce, src);

    // Build and send the response
    dh_response1_chunk response;
    response.group = data.group;
    response.key_min_length = data.key_min_length;
    response.initiator_hashed_nonce = data.initiator_hashed_nonce;
    response.responder_nonce = responder_nonce;
    response.responder_dh_public_key = hostkey->public_key;
    response.responder_challenge_cookie = challenge_cookie;
    // Don't offer responder's identity (eid, public key and signature) for now.
    send(magic(), response, src);
}

/**
 * We got init2, this means the init1/response1 phase might have been done.
 * Find the key_initiator for this exchange and continue.
 */
void key_responder::got_dh_init2(const dh_init2_chunk& data, const link_endpoint& src)
{
    logger::debug() << "Got dh_init2";
    ssu::negotiation::dh_group_type group = ssu::negotiation::dh_group_type::dh_group_1024;
    int keylen = 16;
    byte_array initiator_hashed_nonce;
    byte_array responder_nonce;
    byte_array initiator_dh_public_key;
    byte_array responder_dh_public_key;
    byte_array peer_eid;
    byte_array hash = calc_signature_hash(group, keylen, initiator_hashed_nonce, responder_nonce, initiator_dh_public_key, responder_dh_public_key, peer_eid);
}

/**
 * We got a response, this means we might've sent a request first, find the corresponding initiator.
 */
void key_responder::got_dh_response1(const dh_response1_chunk& data, const link_endpoint& src)
{
    key_initiator* initiator = host_.lock()->get_initiator(data.initiator_hashed_nonce).get();
    if (!initiator or initiator->group() != data.group)
        return warning("Got dh_response1 for unknown dh_init1");
    if (initiator->is_done())
        return warning("Got duplicate dh_response1 for completed initiator");

    logger::debug() << "Got dh_response1";

    initiator->send_dh_init2();
}

//===========================================================================================================
// key_initiator
//===========================================================================================================

key_initiator::key_initiator(const link_endpoint& target)
    : to(target)
    , transmit_timer(host_.lock().get())
{
}

void key_initiator::send_dh_init1()
{
    logger::debug() << "Send dh_init1 to " << to;
    state_ = state::init1;

    // Clear previous initiator state in case it was after init1, we're restarting the init.
    // .... TODO ....

    // Construct dh_init1 frame from the current state.
    dh_hostkey_t* hostkey = host_.lock()->get_dh_key(dh_group); // get or generate a host key

    dh_init1_chunk init;
    init.group = dh_group;
    init.key_min_length = key_min_length;//?
    init.initiator_hashed_nonce = initiator_hashed_nonce;
    init.initiator_dh_public_key = hostkey->public_key;
    init.responder_eid.clear();

    send(magic(), init, to);
}

void key_initiator::send_dh_init2()
{
    logger::debug() << "Send dh_init2 to " << to;
    state_ = state::init2;
}

} // namespace negotiation

//===========================================================================================================
// key_host_state
//===========================================================================================================

std::shared_ptr<ssu::negotiation::key_initiator> key_host_state::get_initiator(byte_array nonce)
{
    auto it = dh_initiators_.find(nonce);
    if (it == dh_initiators_.end()) {
        return 0;
    }
    return it->second.lock();
}

} // namespace ssu
