//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "opus_decode_sink.h"

using namespace std;

namespace voicebox {

void opus_decode_sink::set_enabled(bool enabling)
{
    if (enabling and !is_enabled())
    {
        assert(!decstate);
        int error = 0;
        decstate = opus_decoder_create(sample_rate(), num_channels(), &error);
        assert(decstate);
        assert(!error);

        int framesize, rate;
        opus_decoder_ctl(decstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms
        set_frame_size(framesize);
        set_sample_rate(rate);
        logger::debug() << "opus_output: frame size " << framesize << " sample rate " << rate;

        super::set_enabled(true);
    }
    else if (!enabling and is_enabled())
    {
        super::set_enabled(false);

        assert(decstate);
        opus_decoder_destroy(decstate);
        decstate = nullptr;
    }
}

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
        logger::debug() << "Decode frame size: " << bytebuf.size();
        int len = opus_decode_float(decstate, (unsigned char*)bytebuf.data(),
            bytebuf.size(), samplebuf.as<float>(), frame_size(), /*decodeFEC:*/0);
        if (len < 0) {
            // perform recovery - fill buffer with 0 for example...
        }
        assert(len == (int)frame_size());
    }
    else
    {
        // "decode" a missing frame
        int len = opus_decode_float(decstate, NULL, 0, samplebuf.as<float>(),
            frame_size(), /*decodeFEC:*/0);
        assert(len > 0);
    }
}

} // voicebox namespace
