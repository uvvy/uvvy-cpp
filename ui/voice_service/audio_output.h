#pragma once

#include <vector>
#include "packetized_output.h"

/**
 * This class represents a high-level sink for audio output
 * to the currently selected output device, at a controllable bitrate,
 * providing automatic queueing and interthread synchronization.
 */
class audio_output : public packetized_output
{
    typedef packetized_output super;

public:
    audio_output() = default;
    audio_output(int framesize, double samplerate, int channels = 1)
        : packetized_output(framesize, samplerate, channels)
    {}

    int write_frames(const float *buf, int nframes);
    int write_frames(const std::vector<float> &buf);

protected:
    // Produce a frame of audio data to be sent to the sound hardware.
    // The default implementation produces data from the output queue,
    // but it may be overridden to produce data some other way
    // directly in the context of the separate audio thread.
    // The subclass must handle multithread synchronization in this case!
    void produce_output(float *buf) override;
};
