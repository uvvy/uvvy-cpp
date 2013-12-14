#include "logging.h"
#include "audio_hardware.h"
#include "abstract_audio_output.h"

abstract_audio_output::abstract_audio_output(int framesize, double samplerate, int channels)
{
    set_frame_size(framesize);
    set_sample_rate(samplerate);
    set_num_channels(channels);
}

void abstract_audio_output::set_enabled(bool enabling)
{
    logger::debug() << __PRETTY_FUNCTION__ << " " << enabling;
    if (enabling and !is_enabled())
    {
        if (frame_size() <= 0 or sample_rate() <= 0.0)
        {
            logger::warning() << "Bad frame size " << frame_size()
                << " or sample rate " << sample_rate();
            return;
        }

        bool wasempty = audio_hardware::add_outstream(this);
        super::set_enabled(true);
        if (wasempty or audio_hardware::get_sample_rate() < sample_rate()) {
            audio_hardware::reopen(); // (re-)open at suitable rate
        }
    } 
    else if (!enabling and is_enabled())
    {
        bool isempty = audio_hardware::remove_outstream(this);
        super::set_enabled(false);
        if (isempty) {
            audio_hardware::reopen(); // close or reopen without output
        }
    }
}

void abstract_audio_output::get_output(float *buf)
{
    assert(sample_rate() == audio_hardware::get_sample_rate());   // XXX
    assert(frame_size() == audio_hardware::get_frame_size());
    produce_output(buf);
}
