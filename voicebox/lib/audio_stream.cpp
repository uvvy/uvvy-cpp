//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <cassert>
#include <boost/log/trivial.hpp>
#include "voicebox/audio_stream.h"

audio_stream::audio_stream(int framesize, double samplerate, int channels)
{
    set_frame_size(framesize);
    set_sample_rate(samplerate);
    set_num_channels(channels);
}

audio_stream::~audio_stream()
{
    disable();
}

void audio_stream::set_enabled(bool enabling)
{
    BOOST_LOG_TRIVIAL(debug) << __PRETTY_FUNCTION__ << " " << enabling;
    enabled_ = enabling;
}

void audio_stream::set_frame_size(unsigned int frame_size)
{
    assert(!is_enabled());
    frame_size_ = frame_size;
}

void audio_stream::set_sample_rate(double rate)
{
    assert(!is_enabled());
    rate_ = rate;
}

void audio_stream::set_num_channels(unsigned int num_channels)
{
    assert(!is_enabled());
    num_channels_ = num_channels;
}
