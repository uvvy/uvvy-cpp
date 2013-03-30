//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "link.h"
#include "host.h"
#include "negotiation/key_message.h"

namespace ssu {
namespace negotiation {

/**
 * This abstract base class manages the responder side of the key exchange.
 * It uses link_receiver interface as base to receive negotiation protocol control messages
 * and respond to incoming key exchange requests.
 */
class key_responder : public link_receiver
{
    std::weak_ptr<host> host_;

    void got_dh_init1(const dh_init1_chunk& data, const link_endpoint& src);

public:
    virtual void receive(const byte_array& msg, const link_endpoint& src);


};

} // namespace negotiation
} // namespace ssu
