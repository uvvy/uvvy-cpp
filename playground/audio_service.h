//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "host.h"

/**
 * Audio service creates several streams for sending and receiving packetized audio data
 * as well as a special high-priority stream for controlling the session establishment
 * and sending audio session control commands.
 */
class audio_service
{
    const std::string service_name{"streaming"};
    const std::string protocol_name{"opus"};

    class private_impl;
    std::shared_ptr<private_impl> pimpl_;

public:
    audio_service(std::shared_ptr<ssu::host> host);
    ~audio_service();

    void establish_outgoing_session(ssu::peer_id const& eid, std::vector<std::string> ep_hints);
    void listen_incoming_session();
};
