//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/log/trivial.hpp>
#include "voicebox/packet_sink.h"

using namespace std;
using namespace sss;

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
        buffer.as<big_uint32_t>()[2] = sequence_number_++;
        stream_->write_datagram(buffer, stream::datagram_type::non_reliable);
    }
}

void packet_sink::send_packets()
{
    if (!stream_->is_connected()) {
        BOOST_LOG_TRIVIAL(warning) << "Packet sink - stream is not connected, cannot send";
        return;
    }

    byte_array buffer;
    do {
        produce_output(buffer);
    } while(!buffer.is_empty());
}

} // voicebox namespace
