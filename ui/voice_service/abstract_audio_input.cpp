#include "abstract_audio_input.h"

abstract_audio_input::abstract_audio_input(int framesize, double samplerate, int channels)
{
    set_frame_size(framesize);
    set_sample_rate(samplerate);
    set_num_channels(channels);
}

void abstract_audio_input::set_enabled(bool enabling)
{
    super::set_enabled(enabling);
}
