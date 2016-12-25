//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "CallService.h"

using namespace std;
using namespace sss;

CallService::CallService(HostState& s, QObject *parent)
    : QObject(parent)
    , audioclient_(s.host())
{
    audioclient_.on_session_started.connect([this] {
        // Less detailed logging while in real-time
        logger::core::get()->set_filter(logger::trivial::severity >= logger::trivial::warning);
        // Todo: post this to GUI thread? Signals work fine across threads boundaries.
        emit callStarted();
    });

    audioclient_.on_session_finished.connect([this] {
        // More detailed logging while not in real-time
        logger::core::get()->set_filter(logger::trivial::severity >= logger::trivial::debug);
        // Todo: post this to GUI thread? Signals work fine across threads boundaries.
        emit callFinished();
    });

    audioclient_.listen_incoming_session();

    // connect(actionCall, SIGNAL(triggered()), this, SLOT(call()));
}

void CallService::makeCall(QString callee_eid)
{
    if (isCallActive())
        return;

    audioclient_.establish_outgoing_session(
        string(callee_eid.toUtf8().constData()), vector<string>());
}

void CallService::hangUp()
{
    if (isCallActive()) {
        audioclient_.end_session();
    }
}

bool CallService::isCallActive() const
{
    return audioclient_.is_active();
}

