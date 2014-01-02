//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "voicebox/packet_sink.h"

using namespace std;
using namespace ssu;

namespace voicebox {

/**
 * Somebody needs to pull this method to send the packets,
 * some kind of signal on_ready_send() or something is necessary.
 */
void packet_sink::produce_output(byte_array& buffer)
{
    // Get data from our producer, if any.
    if (producer()) {
        producer()->produce_output(buffer);
    }

    if (!buffer.is_empty()) {
        stream_->write_datagram(buffer, stream::datagram_type::non_reliable);
    }
}

void packet_sink::send_packets()
{
    if (!stream_->is_connected()) {
        logger::warning() << "Packet sink - stream is not connected, cannot send";
        return;
    }

    byte_array buffer;
    do {
        produce_output(buffer);
    } while(!buffer.is_empty());
}

} // voicebox namespace
