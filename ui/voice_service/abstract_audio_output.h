#pragma once

#include "audio_stream.h"

/**
 * This abstract base class represents a sink for audio output
 * to the currently selected output device, at a controllable bitrate.
 */
class abstract_audio_output : public audio_stream
{
    typedef audio_stream super;
public:
    abstract_audio_output() = default;
    abstract_audio_output(int framesize, double samplerate);

    void set_enabled(bool enabled) override;

protected:
    /**
     * Produce a frame of audio data to be sent to the sound hardware.
     * This method is typically called from a dedicated audio thread,
     * so the subclass must handle multithread synchronization!
     */
    virtual void produce_output(float *buf) = 0;

private:
    /**
     * Get output from client, resampling and/or buffering it as necessary
     * to match hwrate and hwframesize (in audio.cc).
     */
    void get_output(float *buf);
};
