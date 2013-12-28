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

namespace voicebox {

packetizer::packetizer()
{
    queue_.on_ready_read.connect([this] { on_ready_read(); });
    queue_.on_queue_empty.connect([this] { on_queue_empty(); });
}

void packetizer::produce_output(byte_array& buffer)
{}

void packetizer::accept_input(byte_array data)
{}

} // voicebox namespace
