#pragma once

#include <vector>
#include "packetized_input.h"

/**
 * This class represents a high-level source of audio input
 * from the currently selected input device, at a controllable bitrate,
 * providing automatic queueing and interthread synchronization.
 */
class audio_input : public packetized_input
{
    typedef packetized_input super;

public:
    audio_input() = default;
    audio_input(int framesize, double samplerate, int channels = 1);
    ~audio_input();

    int read_frames_available();
    int read_frames(float *buf, int maxframes);
    std::vector<float> read_frames(int maxframes = 1 << 30);

protected:
    // Accept audio input data received from the sound hardware.
    // The default implementation just adds the data to the input queue,
    // but this may be overridden to process the input data
    // directly in the context of the separate audio thread.
    // The subclass must handle multithread synchronization in this case!
    void accept_input(byte_array frame) override;

private:
    void read_into(float *buf, int nframes);
};
