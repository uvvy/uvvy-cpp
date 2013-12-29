//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "voicebox/opus_decode_sink.h"

using namespace std;

namespace voicebox {

void opus_decode_sink::set_enabled(bool enabling)
{
    logger::debug() << __PRETTY_FUNCTION__ << " " << enabling;
    if (enabling and !is_enabled())
    {
        assert(!decode_state_);
        int error = 0;
        decode_state_ = opus_decoder_create(sample_rate(), num_channels(), &error);
        assert(decode_state_);
        assert(!error);

        int framesize, rate;
        opus_decoder_ctl(decode_state_, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms
        set_frame_size(framesize);
        set_sample_rate(rate);
        logger::debug() << "opus_decode_sink: frame size "
            << dec << framesize << " sample rate " << rate;

        super::set_enabled(true);
    }
    else if (!enabling and is_enabled())
    {
        super::set_enabled(false);

        assert(decode_state_);
        opus_decoder_destroy(decode_state_);
        decode_state_ = nullptr;
    }
}

// Packets format:
// [8 bytes] microseconds since epoch (Jan 1, 2010)
// [4 bytes] sequence number
// [variable] payload
const int buffer_offset = 12;

void opus_decode_sink::produce_output(byte_array& samplebuf)
{
    // Grab the next buffer from the queue
    byte_array bytebuf;
    if (producer()) {
        producer()->produce_output(bytebuf);
    }

    samplebuf.resize(frame_bytes());

    // Decode the frame
    if (!bytebuf.is_empty())
    {
        assert(bytebuf.size() > buffer_offset);

        logger::debug() << "Decode frame size: " << bytebuf.size() - buffer_offset;
        int len = opus_decode_float(decode_state_, (unsigned char*)bytebuf.data() + buffer_offset,
            bytebuf.size() - buffer_offset, samplebuf.as<float>(), frame_size(), /*decodeFEC:*/0);

        if (len < 0) {
            // perform recovery - fill buffer with 0 for example...
        }
        assert(len == (int)frame_size());
    }
    else
    {
        // "decode" a missing frame
        int len = opus_decode_float(decode_state_, NULL, 0, samplebuf.as<float>(),
            frame_size(), /*decodeFEC:*/0);
        assert(len > 0);
    }
}

} // voicebox namespace
