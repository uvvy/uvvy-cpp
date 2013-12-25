//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "audio_source.h"
#include "audio_sink.h"
#include "synchronized_queue.h"

namespace voicebox {

/**
 * It doesn't have, neither use, acceptor or producer, since it sits at the intersection
 * of push and pull threads, all other components call into jitterbuffer.
 * It passes the data on, using synchronization.
 *
 * Jitterbuffer forwards queue signals for use by other layers.
 */
class jitterbuffer : public audio_source, public audio_sink
{
protected:
    synchronized_queue<byte_array> queue_;
    uint32_t sequence_number_{0};

public:
    jitterbuffer();

    void produce_output(byte_array& buffer) override; // from sink
    void accept_input(byte_array data) override; // from source

    synchronized_queue<byte_array>::state_signal on_ready_read;
    synchronized_queue<byte_array>::state_signal on_queue_empty;
};

}
