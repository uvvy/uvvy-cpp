#include "audio_loopback.h"

// @todo Obtain these from audio_monitor
#define monFrameSize 480
#define monSampleRate 48000.0

audio_loopback::audio_loopback()
    : audio_stream()
{
    in_.on_ready_read.connect([this] { in_ready_read(); });
}

void audio_loopback::set_loopback_delay(float secs)
{
    assert(!is_enabled());
    assert(secs > 0.0);
    delay_ = secs;
}

void audio_loopback::set_enabled(bool enabling)
{
    if (enabling and !is_enabled())
    {
        int framesize = frame_size() ? frame_size() : monFrameSize;
        in_.set_frame_size(framesize);
        out_.set_frame_size(framesize);

        int samplerate = sample_rate() ? sample_rate() : monSampleRate;
        in_.set_sample_rate(samplerate);
        out_.set_sample_rate(samplerate);

        // Enable output when we have enough audio queued
        threshold_ = (int)(delay_ * in_.sample_rate() / in_.frame_size());

        in_.enable();
        super::set_enabled(true);
    }
    else if (!enabling and is_enabled())
    {
        super::set_enabled(false);
        in_.reset();
        out_.reset();
    }
}

void audio_loopback::in_ready_read()
{
    out_.write_frames(in_.read_frames());
    if (!out_.is_enabled() and out_.num_frames_queued() >= threshold_) {
        out_.enable();
        on_start_playback();
    }
}

