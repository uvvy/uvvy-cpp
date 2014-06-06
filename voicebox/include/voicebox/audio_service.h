//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/signals2/signal.hpp>
#include "ssu/host.h"

// Set to 1 if you want to console-log in realtime thread.
#define REALTIME_CRIME 0

#define TRACE_RARE 199
#define TRACE_ENTRY 250
#define TRACE_DETAIL 251

namespace voicebox {

/**
 * Clock reference base.
 */
static const boost::posix_time::ptime epoch{boost::gregorian::date(2010, boost::gregorian::Jan, 1)};

/**
 * Audio service creates several streams for sending and receiving packetized audio data
 * as well as a special high-priority stream for controlling the session establishment
 * and sending audio session control commands.
 */
class audio_service
{
    class private_impl;
    std::shared_ptr<private_impl> pimpl_;

public:
    audio_service(std::shared_ptr<ssu::host> host);
    ~audio_service();

    bool is_active() const;
    void establish_outgoing_session(ssu::peer_id const& eid, std::vector<std::string> ep_hints);
    void listen_incoming_session();
    /**
     * Terminate an active session, causing all audio I/O to stop.
     * Notifies the far end to also terminate session.
     */
    void end_session();

    // Voice service media signals
    typedef boost::signals2::signal<void (void)> session_signal;
    session_signal on_session_started;
    session_signal on_session_finished;
};

} // voicebox namespace

