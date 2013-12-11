#pragma once

#include "audio_stream.h"

/**
 * This abstract base class represents a source of audio input
 * from the currently selected input device, at a controllable bitrate.
 */
class abstract_audio_input : public audio_stream
{
    typedef audio_stream super;
public:
    abstract_audio_input() = default;
    abstract_audio_input(int framesize, double samplerate);

    void set_enabled(bool enabled) override;

protected:
    /**
     * Accept audio input data received from the sound hardware.
     * This method is typically called from a dedicated audio thread,
     * so the subclass must handle multithread synchronization!
     */
    virtual void accept_input(const float *buf) = 0;
};
