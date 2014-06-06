//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "arsenal/logging.h"
#include "voicebox/packetizer.h"
#include "voicebox/audio_service.h"

namespace voicebox {

packetizer::~packetizer()
{
    audio_source::disable();
    // audio_sink::disable();
}

void packetizer::produce_output(byte_array& buffer)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "packetizer::produce_output";
#endif
    std::unique_lock<std::mutex> guard(mutex_);
    if (queue_.empty()) {
        buffer.resize(0);
        return;
    }

    buffer = queue_.front();
    queue_.pop_front();
    bool emptied = queue_.empty();
    guard.unlock();

    if (emptied) {
        on_queue_empty();
    }
}

void packetizer::accept_input(byte_array data)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "packetizer::accept_input of "
        << std::dec << data.size() << " bytes";
#endif
    std::unique_lock<std::mutex> guard(mutex_);
    bool wasempty = queue_.empty();
    queue_.emplace_back(data);
    guard.unlock();

    // Notify reader if appropriate
    if (wasempty) {
        on_ready_read();
    }
}

} // voicebox namespace
