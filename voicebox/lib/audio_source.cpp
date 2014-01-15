//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "arsenal/logging.h"
#include "voicebox/audio_source.h"

namespace voicebox {

void audio_source::accept_input(byte_array buffer)
{
    if (acceptor_) {
        acceptor_->accept_input(buffer);
    }
}

void audio_source::set_enabled(bool enabling)
{
    logger::debug() << __PRETTY_FUNCTION__ << " " << enabling;
    super::set_enabled(enabling);
    if (acceptor_) {
        acceptor_->set_enabled(enabling);
    }
}

void audio_source::set_frame_size(unsigned int frame_size)
{
    super::set_frame_size(frame_size);
    if (acceptor_) {
        acceptor_->set_frame_size(frame_size);
    }
}

void audio_source::set_sample_rate(double rate)
{
    super::set_sample_rate(rate);
    if (acceptor_) {
        acceptor_->set_sample_rate(rate);
    }
}

void audio_source::set_num_channels(unsigned int num_channels)
{
    super::set_num_channels(num_channels);
    if (acceptor_) {
        acceptor_->set_num_channels(num_channels);
    }
}

}
