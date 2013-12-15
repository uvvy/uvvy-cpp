#include "audio_source.h"

namespace voicebox {

void audio_source::accept_input(byte_array buffer)
{
    if (acceptor_) {
        acceptor_->accept_input(buffer);
    }
}

void audio_source::set_enabled(bool enable)
{
    super::set_enabled(enable);
    if (acceptor_) {
        acceptor_->set_enabled(enable);
    }
}

void audio_source::set_frame_size(int frame_size)
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

void audio_source::set_num_channels(int num_channels)
{
    super::set_num_channels(num_channels);
    if (acceptor_) {
        acceptor_->set_num_channels(num_channels);
    }
}

}
