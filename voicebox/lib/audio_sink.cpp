//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "arsenal/logging.h"
#include "voicebox/audio_sink.h"

namespace voicebox {

void audio_sink::produce_output(byte_array& buffer)
{
    if (producer_) {
        producer_->produce_output(buffer);
    }
}

void audio_sink::set_enabled(bool enabling)
{
    logger::debug() << __PRETTY_FUNCTION__ << " " << enabling;
    super::set_enabled(enabling);
    if (producer_) {
        producer_->set_enabled(enabling);
    }
}

void audio_sink::set_frame_size(unsigned int frame_size)
{
    super::set_frame_size(frame_size);
    if (producer_) {
        producer_->set_frame_size(frame_size);
    }
}

void audio_sink::set_sample_rate(double rate)
{
    super::set_sample_rate(rate);
    if (producer_) {
        producer_->set_sample_rate(rate);
    }
}

void audio_sink::set_num_channels(unsigned int num_channels)
{
    super::set_num_channels(num_channels);
    if (producer_) {
        producer_->set_num_channels(num_channels);
    }
}

}
