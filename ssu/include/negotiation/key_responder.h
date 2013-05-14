//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <memory>
#include "link.h"
#include "crypto.h"
#include "negotiation/key_message.h"

namespace ssu {

class host;

namespace negotiation {

/**
 * This abstract base class manages the responder side of the key exchange.
 * It uses link_receiver interface as base to receive negotiation protocol control messages
 * and respond to incoming key exchange requests.
 *
 * It forwards received requests to a corresponding key initiator in the host state
 * (via host_->get_initiator()).
 */
class key_responder : public link_receiver
{
    std::weak_ptr<host> host_;

    void got_dh_init1(const dh_init1_chunk& data, const link_endpoint& src);
    void got_dh_response1(const dh_response1_chunk& data, const link_endpoint& src);
    void got_dh_init2(const dh_init2_chunk& data, const link_endpoint& src);
    void got_dh_response2(const dh_response2_chunk& data, const link_endpoint& src);

public:
    virtual void receive(const byte_array& msg, const link_endpoint& src);
};

/**
 * Key initiator maintains host state with respect to initiated key exchanges.
 * One initiator keeps state about key exchange with one peer.
 */
class key_initiator
{
    std::weak_ptr<host> host_;
    const link_endpoint& to;
    enum class state {
        init1, init2, done
    } state_;

    // AES-based encryption negotiation state
    ssu::negotiation::dh_group_type dh_group;
    int key_min_length{0};
    boost::array<uint8_t, crypto::hash::size> initiator_hashed_nonce;

    magic_t magic_{0};

protected:
    inline magic_t magic() const { return magic_; }

public:
    key_initiator(const link_endpoint& target) : to(target) {}
    ~key_initiator();

    inline ssu::negotiation::dh_group_type group() const { return dh_group; }
    inline bool is_done() const { return state_ == state::done; }

    void send_dh_init1();
    void send_dh_response1();
    void send_dh_init2();
    void send_dh_response2();
};

} // namespace negotiation

/**
 * Mixin for host state that manages the key exchange state.
 */
class key_host_state
{
    //std::unordered_map<chk_ep, std::weak_ptr<key_initiator>> chk_initiators;
    std::unordered_map<byte_array, std::weak_ptr<negotiation::key_initiator>> dh_initiators_;
    // std::unordered_multimap<endpoint, std::weak_ptr<key_initiator>> ep_initiators;

public:
    std::shared_ptr<ssu::negotiation::key_initiator> get_initiator(byte_array nonce);
};

} // namespace ssu
