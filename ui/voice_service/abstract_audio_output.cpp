#include "abstract_audio_output.h"

abstract_audio_output::abstract_audio_output(int framesize, double samplerate, int channels)
{
    set_frame_size(framesize);
    set_sample_rate(samplerate);
    set_num_channels(channels);
}

void abstract_audio_output::set_enabled(bool enabling)
{
    super::set_enabled(enabling);
}
