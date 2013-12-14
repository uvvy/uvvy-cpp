//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "audio_sender.h"
#include "audio_service.h"

namespace pt = boost::posix_time;
using namespace std;
using namespace ssu;

static const pt::ptime epoch{boost::gregorian::date(1970, boost::gregorian::Jan, 1)};

constexpr int audio_sender::nChannels;

audio_sender::audio_sender(shared_ptr<host> host)
    : strand_(host->get_io_service())
{
    int error{0};
    encode_state_ = opus_encoder_create(48000, nChannels, OPUS_APPLICATION_VOIP, &error);
    assert(encode_state_);
    assert(!error);

    opus_encoder_ctl(encode_state_, OPUS_GET_SAMPLE_RATE(&rate_));
    frame_size_ = rate_ / 100; // 10ms

    opus_encoder_ctl(encode_state_, OPUS_SET_BITRATE(OPUS_AUTO));
    opus_encoder_ctl(encode_state_, OPUS_SET_VBR(1));
    opus_encoder_ctl(encode_state_, OPUS_SET_DTX(1));
}

audio_sender::~audio_sender()
{
    if (stream_) {
        stream_->shutdown(stream::shutdown_mode::write);
    }
    opus_encoder_destroy(encode_state_); encode_state_ = 0;
}

/**
 * Use given stream for sending out audio packets.
 * @param stream Stream handed in from server->accept().
 */
void audio_sender::streaming(shared_ptr<stream> stream)
{
    stream_ = stream;
}

// Called by rtaudio callback to encode and send packet.
void audio_sender::send_packet(float* buffer, size_t nFrames)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "send_packet frame size " << frame_size_
        << ", got nFrames " << nFrames;
#endif
    assert((int)nFrames == frame_size_);
    byte_array samplebuf(nFrames*sizeof(float)+8);

    // Timestamp the packet with our own clock reading.
    int64_t ts = (pt::microsec_clock::universal_time() - epoch).total_milliseconds();
    samplebuf.as<big_int64_t>()[0] = ts;
    // Ideally, an ack packet would contain ts info at the receiving side for this packet.

    // @todo Perform encode in a separate thread, not capture thread.
    //async{
    opus_int32 nbytes = opus_encode_float(encode_state_, buffer, nFrames,
        (unsigned char*)samplebuf.data()+8, nFrames*sizeof(float));
    assert(nbytes > 0);
    samplebuf.resize(nbytes+8);
    logger::file_dump dump(samplebuf, "encoded opus packet");
    strand_.post([this, samplebuf]{
        stream_->write_datagram(samplebuf, stream::datagram_type::non_reliable);
    });
    // }
}
