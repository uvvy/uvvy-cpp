//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/endian/conversion.hpp>
#include <boost/optional/optional.hpp>
#include "logging.h"
#include "xdr.h"
#include "protocol.h" // for to_underlying XXX

namespace ssu {
namespace negotiation {

/**
All the XDR crap for key negotiation:
////////// Lightweight Checksum Negotiation //////////

// Checksum negotiation chunks
struct KeyChunkChkI1Data {
    // XXX nonces should be 64-bit, to ensure USIDs unique over all time!
    unsigned int    cki;        // Initiator's checksum key
    unsigned char   chani;      // Initiator's channel number
    opaque      cookie<>;   // Responder's cookie, if any
    opaque      ulpi<>;     // Upper-level protocol data
    opaque      cpkt<>;     // Piggybacked channel packet
};
struct KeyChunkChkR1Data {
    unsigned int    cki;        // Initiator's checksum key, echoed
    unsigned int    ckr;        // Responder's checksum key,
                    // = 0 if cookie required
    unsigned char   chanr;      // Responder's channel number,
                    // 0 if cookie required
    opaque      cookie<>;   // Responder's cookie, if any
    opaque      ulpr<>;     // Upper-level protocol data
    opaque      cpkt<>;     // Piggybacked channel packet
};

////////// Diffie-Helman Key Negotiation //////////

// Encrypted and authenticated identity blocks for I2 and R2 messages
struct KeyIdentI {
    unsigned char   chani;      // Initiator's channel number
    opaque      eidi<256>;  // Initiator's endpoint identifier
    opaque      eidr<256>;  // Desired EID of responder
    opaque      idpki<>;    // Initiator's identity public key
    opaque      sigi<>;     // Initiator's parameter signature
    opaque      ulpi<>;     // Upper-level protocol data
};
struct KeyIdentR {
    unsigned char   chanr;      // Responder's channel number
    opaque      eidr<256>;  // Responder's endpoint identifier
    opaque      idpkr<>;    // Responder's identity public key
    opaque      sigr<>;     // Responder's parameter signature
    opaque      ulpr<>;     // Upper-level protocol data
};

// Responder DH key signing block for R2 messages (JFKi only)
struct KeyParamR {
    DhGroup     group;      // DH group for public keys
    opaque      dhr<384>;   // Responder's DH public key
};

// Key parameter signing block for I2 and R2 messages
struct KeyParams {
    DhGroup     group;      // DH group for public keys
    int     keylen;     // AES key length: 16, 24, or 32
    opaque      nhi[32];    // Initiator's hashed nonce
    opaque      nr[32];     // Responder's nonce
    opaque      dhi<384>;   // Initiator's DH public key
    opaque      dhr<384>;   // Responder's DH public key
    opaque      eid<256>;   // Peer's endpoint identifier
};

union KeyChunkUnion switch (KeyChunkType type) {
    case KeyChunkPacket:    opaque packet<>;

    case KeyChunkChkI1: KeyChunkChkI1Data chki1;
    case KeyChunkChkR1: KeyChunkChkR1Data chkr1;

    case KeyChunkDhI1:  KeyChunkDhI1Data dhi1;
    case KeyChunkDhR1:  KeyChunkDhR1Data dhr1;
    case KeyChunkDhI2:  KeyChunkDhI2Data dhi2;
    case KeyChunkDhR2:  KeyChunkDhR2Data dhr2;
};
typedef KeyChunkUnion ?KeyChunk;


// Top-level format of all negotiation protocol messages
struct KeyMessage {
    int     magic;      // 24-bit magic value
    KeyChunk    chunks<>;   // Message chunks
};

*/

class packet_chunk
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int) {
    }
};

class checksum_init_chunk
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int) {
    }
};

class checksum_response_chunk
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int) {
    }
};

enum class dh_group_type : uint32_t {
    dh_group_1024 = 0x01, // 1024-bit DH group
    dh_group_2048 = 0x02, // 2048-bit DH group
    dh_group_3072 = 0x03  // 3072-bit DH group
};

// DH/JFK negotiation chunks

class dh_init1_chunk
{
    public://temp
    dh_group_type  group;                       // DH group of initiator's public key
    uint32_t       key_min_length;              // Minimum AES key length: 16, 24, 32
    byte_array     initiator_hashed_nonce;      // Initiator's SHA256-hashed nonce
    byte_array     initiator_dh_public_key;     // Initiator's DH public key
    byte_array     responder_eid;               // Optional: desired EID of responder

    dh_init1_chunk()
        : group(dh_group_type::dh_group_1024)
        , key_min_length(0)
    {}

    friend class boost::serialization::access;
    template<class Archive>
    void load(Archive &ar, const unsigned int) {
        uint32_t grp;
        ar >> grp;
        boost::endian::big_to_native(grp);
        group = dh_group_type(grp);
        ar >> key_min_length;
        boost::endian::big_to_native(key_min_length);
        xdr::decode_vector(ar, initiator_hashed_nonce, 32);
        xdr::decode_array(ar, initiator_dh_public_key, 384);
        xdr::decode_array(ar, responder_eid, 256);
    }

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive &ar, const unsigned int) const {
        uint32_t grp = uint32_t(group);
        boost::endian::native_to_big(grp);
        ar << grp;
        uint32_t kml = key_min_length;
        boost::endian::native_to_big(kml);
        ar << kml;
        xdr::encode_vector(ar, initiator_hashed_nonce, 32);
        xdr::encode_array(ar, initiator_dh_public_key, 384);
        xdr::encode_array(ar, responder_eid, 256);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

class dh_response1_chunk
{
    dh_group_type  group;                       // DH group for public keys
    int            key_min_length;              // Chosen AES key length: 16, 24, 32
    byte_array     initiator_hashed_nonce;      // Initiator's hashed nonce
    byte_array     responder_nonce;             // Responder's nonce
    byte_array     responder_dh_public_key;     // Responder's DH public key
    byte_array     responder_challenge_cookie;  // Responder's challenge cookie
    byte_array     responder_eid;               // Optional: responder's EID
    byte_array     responder_public_key;        // Optional: responder's public key
    byte_array     responder_signature;         // Optional: responder's signature

    friend class boost::serialization::access;
    template<class Archive>
    void load(Archive &ar, const unsigned int) {
        ar & group;
        ar & key_min_length;
        xdr::decode_vector(ar, initiator_hashed_nonce, 32);
        xdr::decode_vector(ar, responder_nonce, 32);
        xdr::decode_array(ar, responder_dh_public_key, 384);
        xdr::decode_array(ar, responder_challenge_cookie, 256);
        xdr::decode_array(ar, responder_eid, 256);
        xdr::decode_array(ar, responder_public_key, ~0);
        xdr::decode_array(ar, responder_signature, ~0);
    }
    template<class Archive>
    void save(Archive &ar, const unsigned int) const {
        ar & group;
        ar & key_min_length;
        xdr::encode_vector(ar, initiator_hashed_nonce, 32);
        xdr::encode_vector(ar, responder_nonce, 32);
        xdr::encode_array(ar, responder_dh_public_key, 384);
        xdr::encode_array(ar, responder_challenge_cookie, 256);
        xdr::encode_array(ar, responder_eid, 256);
        xdr::encode_array(ar, responder_public_key, ~0);
        xdr::encode_array(ar, responder_signature, ~0);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

class dh_init2_chunk
{
    dh_group_type  group;                       // DH group for public keys
    int            key_min_length;              // AES key length: 16, 24, or 32
    byte_array     initiator_nonce;             // Initiator's original nonce
    byte_array     responder_nonce;             // Responder's nonce
    byte_array     initiator_dh_public_key;     // Initiator's DH public key
    byte_array     responder_dh_public_key;     // Responder's DH public key
    byte_array     responder_challenge_cookie;  // Responder's challenge cookie
    byte_array     initiator_id;                // Initiator's encrypted identity

    friend class boost::serialization::access;
    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
        ar & group;
        ar & key_min_length;
        xdr::decode_vector(ar, initiator_nonce, 32);
        xdr::decode_vector(ar, responder_nonce, 32);
        xdr::decode_array(ar, initiator_dh_public_key, 384);
        xdr::decode_array(ar, responder_dh_public_key, 384);
        xdr::decode_array(ar, responder_challenge_cookie, 256);
        xdr::decode_array(ar, initiator_id, ~0);
    }
    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
        ar & group;
        ar & key_min_length;
        xdr::encode_vector(ar, initiator_nonce, 32);
        xdr::encode_vector(ar, responder_nonce, 32);
        xdr::encode_array(ar, initiator_dh_public_key, 384);
        xdr::encode_array(ar, responder_dh_public_key, 384);
        xdr::encode_array(ar, responder_challenge_cookie, 256);
        xdr::encode_array(ar, initiator_id, ~0);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

class dh_response2_chunk
{
    byte_array  initiator_hashed_nonce;         // Initiator's original nonce
    byte_array  responder_id;                   // Responder's encrypted identity

    friend class boost::serialization::access;
    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
        xdr::decode_vector(ar, initiator_hashed_nonce, 32);
        xdr::decode_array(ar, responder_id, ~0);
    }
    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
        xdr::encode_vector(ar, initiator_hashed_nonce, 32);
        xdr::encode_array(ar, responder_id, ~0);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

enum class key_chunk_type : uint32_t {
    packet             = 0x0001,
    checksum_init      = 0x0011,
    checksum_response  = 0x0012,
    dh_init1           = 0x0021,
    dh_response1       = 0x0022,
    dh_init2           = 0x0023,
    dh_response2       = 0x0024,
};

class key_chunk
{
    public://temp
    key_chunk_type                             type;
    boost::optional<packet_chunk>              packet;
    boost::optional<checksum_init_chunk>       checksum_init;
    boost::optional<checksum_response_chunk>   checksum_response;
    boost::optional<dh_init1_chunk>            dh_init1;
    boost::optional<dh_response1_chunk>        dh_response1;
    boost::optional<dh_init2_chunk>            dh_init2;
    boost::optional<dh_response2_chunk>        dh_response2;

    friend class boost::serialization::access;
    template<class Archive>
    void load(Archive &ar, const unsigned int) {
        uint32_t t;
        ar >> t;
        type = key_chunk_type(boost::endian2::big(t));
        switch (type) {
            case key_chunk_type::packet:
            {
                packet = packet_chunk();
                ar >> *packet;
                break;
            }
            case key_chunk_type::checksum_init:
            {
                checksum_init = checksum_init_chunk();
                ar >> *checksum_init;
                break;
            }
            case key_chunk_type::checksum_response:
            {
                checksum_response = checksum_response_chunk();
                ar >> *checksum_response;
                break;
            }
            case key_chunk_type::dh_init1:
            {
                dh_init1 = dh_init1_chunk();
                ar >> *dh_init1;
                break;
            }
            case key_chunk_type::dh_response1:
            {
                dh_response1 = dh_response1_chunk();
                ar >> *dh_response1;
                break;
            }
            case key_chunk_type::dh_init2:
            {
                dh_init2 = dh_init2_chunk();
                ar >> *dh_init2;
                break;
            }
            case key_chunk_type::dh_response2:
            {
                dh_response2 = dh_response2_chunk();
                ar >> *dh_response2;
                break;
            }
        }
    }
    template<class Archive>
    void save(Archive &ar, const unsigned int) const {
        uint32_t t = boost::endian2::big(to_underlying(type));
        ar << t;
        switch (type) {
            case key_chunk_type::packet:
                ar << *packet;
                break;
            case key_chunk_type::checksum_init:
                ar << *checksum_init;
                break;
            case key_chunk_type::checksum_response:
                ar << *checksum_response;
                break;
            case key_chunk_type::dh_init1:
                ar << *dh_init1;
                break;
            case key_chunk_type::dh_response1:
                ar << *dh_response1;
                break;
            case key_chunk_type::dh_init2:
                ar << *dh_init2;
                break;
            case key_chunk_type::dh_response2:
                ar << *dh_response2;
                break;
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

class key_message
{
    public://temp
    uint32_t magic;
    std::vector<key_chunk> chunks;

    friend class boost::serialization::access;
    template<class Archive>
    void load(Archive &ar, const unsigned int) {
        uint32_t m;
        ar >> m;
        magic = boost::endian2::big(m);
        uint32_t count;
        ar >> count;
        count = boost::endian2::big(count);
        for (uint32_t i = 0; i < count; ++i) // Hmm, potential DOS possibility - send a very big list?
        {
            key_chunk c;
            xdr::decode_option(ar, c, ~0);
            chunks.push_back(c);
        }
    }
    template<class Archive>
    void save(Archive &ar, const unsigned int) const {
        uint32_t m = boost::endian2::big(magic);
        uint32_t count = chunks.size();
        boost::endian::native_to_big(count);
        ar << m;
        ar << count;
        for (auto t : chunks) {
            xdr::encode_option(ar, t, ~0);
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
};

} // namespace negotiation
} // namespace ssu

// Disable versioning for all these classes
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::packet_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::checksum_init_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::checksum_response_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::dh_init1_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::dh_response1_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::dh_init2_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::dh_response2_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::key_chunk, boost::serialization::object_serializable)
BOOST_CLASS_IMPLEMENTATION(ssu::negotiation::key_message, boost::serialization::object_serializable)
