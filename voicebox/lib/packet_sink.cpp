//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "packet_sink.h"

using namespace std;
using namespace ssu;
namespace pt = boost::posix_time;

namespace voicebox {

// static const pt::ptime epoch{boost::gregorian::date(2010, boost::gregorian::Jan, 1)};

// audio_sender::~audio_sender()
// {
//     if (stream_) {
//         stream_->shutdown(stream::shutdown_mode::write);
//     }
// }

// // Called by rtaudio callback to encode and send packet.
// void audio_sender::send_packet(float* buffer, size_t nFrames)
// {
//     byte_array samplebuf(nFrames*sizeof(float)+8);

//     // Timestamp the packet with our own clock reading. -- put in opus_encode_sink (?)
//     int64_t ts = (pt::microsec_clock::universal_time() - epoch).total_milliseconds();
//     samplebuf.as<big_int64_t>()[0] = ts;
//     // Ideally, an ack packet would contain ts info at the receiving side for this packet.
// }

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
        logger::warning() << "packet_sink - Stream is not connected, cannot send";
        return;
    }

    byte_array buffer;
    do {
        produce_output(buffer);
    } while(!buffer.is_empty());
}

} // voicebox namespace
