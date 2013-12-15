#include "audio_stream.h"
#include <cassert>

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

void audio_stream::set_enabled(bool enable)
{
    enabled_ = enable;
}

void audio_stream::set_frame_size(int frame_size)
{
    assert(!is_enabled());
    frame_size_ = frame_size;
}

void audio_stream::set_sample_rate(double rate)
{
    assert(!is_enabled());
    rate_ = rate;
}

void audio_stream::set_num_channels(int num_channels)
{
    assert(!is_enabled());
    num_channels_ = num_channels;
}
