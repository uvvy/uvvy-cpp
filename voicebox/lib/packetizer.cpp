//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "voicebox/packetizer.h"
#include "voicebox/audio_service.h"

namespace voicebox {

packetizer::packetizer(audio_source* from)
{
    queue_.on_ready_read.connect([this] { on_ready_read(); });
    queue_.on_queue_empty.connect([this] { on_queue_empty(); });

    if (from) {
        from->set_acceptor(this);
    }
}

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
    if (queue_.empty()) {
        buffer.resize(0);
        return;
    }
    buffer = queue_.dequeue();
}

void packetizer::accept_input(byte_array data)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "packetizer::accept_input of "
        << std::dec << data.size() << " bytes";
#endif
    queue_.enqueue(data);
}

} // voicebox namespace
