//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <mutex>
#include <deque>
#include <boost/signals2/signal.hpp>
#include "voicebox/audio_source.h"
#include "voicebox/audio_sink.h"

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
    std::mutex mutex_;
    std::deque<byte_array> queue_;
    uint32_t sequence_number_{0};
    int64_t time_skew_{0};

    void reset();
    void enqueue(byte_array a);

public:
    inline jitterbuffer(audio_source* from = nullptr)
    {
        if (from) {
            from->set_acceptor(this);
        }
    }

    void produce_output(byte_array& buffer) override; // from sink
    void accept_input(byte_array data) override; // from source

    typedef boost::signals2::signal<void (void)> state_signal;
    state_signal on_ready_read;
    state_signal on_queue_empty;
};

}
