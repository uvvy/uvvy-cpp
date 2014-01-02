//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "voicebox/opus_encode_sink.h"
#include "voicebox/audio_service.h"

using namespace std;

namespace voicebox {

void opus_encode_sink::set_enabled(bool enabling)
{
    logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__ << " " << enabling;
    if (enabling and !is_enabled()) {
        assert(!encode_state_);
        int error = 0;
        encode_state_ = opus_encoder_create(sample_rate(), num_channels(),
            OPUS_APPLICATION_VOIP, &error);
        assert(encode_state_);
        assert(!error);

        int framesize, rate;
        opus_encoder_ctl(encode_state_, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms
        set_frame_size(framesize);
        set_sample_rate(rate);
        logger::debug(TRACE_DETAIL) << "opus_encode_sink: frame size " << dec << framesize
            << " sample rate " << rate;

        opus_encoder_ctl(encode_state_, OPUS_SET_VBR(1));
        opus_encoder_ctl(encode_state_, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(encode_state_, OPUS_SET_DTX(1));

        super::set_enabled(true);

    } else if (!enabling and is_enabled()) {

        super::set_enabled(false);

        assert(encode_state_);
        opus_encoder_destroy(encode_state_);
        encode_state_ = nullptr;
    }
}

const int ts_size = 8;
const int ts_floats = ts_size / sizeof(float); // Timestamp size in floats, due to cast.
const int buffer_offset = 12;

void opus_encode_sink::produce_output(byte_array& buffer)
{
    // Get data from our producer, if any.
    byte_array samplebuf;
    if (producer()) {
        producer()->produce_output(samplebuf);
    }

    if (!samplebuf.is_empty())
    {
#if REALTIME_CRIME
        logger::debug(TRACE_DETAIL) << "Encoding source frame with size: "
            << dec << samplebuf.size() - ts_size;
#endif
        // Encode the frame and write it into a buffer
        int maxbytes = 1024;//meh, any opus option to get this?
        buffer.resize(maxbytes);
        int nbytes = opus_encode_float(encode_state_, samplebuf.as<const float>() + ts_floats,
            frame_size(),
            buffer.as<unsigned char>() + buffer_offset, buffer.size() - buffer_offset);
        assert(nbytes <= maxbytes);
        buffer.resize(nbytes + buffer_offset);
        buffer.as<big_int64_t>()[0] = samplebuf.as<big_int64_t>()[0]; // Copy timestamp
#if REALTIME_CRIME
        logger::debug(TRACE_DETAIL) << "Encoded frame size: " << dec << nbytes;
        logger::file_dump(buffer, "encoded opus packet");
#endif
    }
    else // packetizer may return empty buffer if queue is empty.
    {
        buffer.resize(0);
    }
}

} // voicebox namespace
