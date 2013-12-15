#include "audio_sink.h"

namespace voicebox {

void audio_sink::produce_output(void* buffer)
{
    if (producer_) {
        producer_->produce_output(buffer);
    }
}

void audio_sink::set_enabled(bool enable)
{
    super::set_enabled(enable);
    if (producer_) {
        producer_->set_enabled(enable);
    }
}

void audio_sink::set_frame_size(int frame_size)
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

void audio_sink::set_num_channels(int num_channels)
{
    super::set_num_channels(num_channels);
    if (producer_) {
        producer_->set_num_channels(num_channels);
    }
}

}
