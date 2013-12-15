#pragma once

#include "audio_stream.h"

namespace voicebox {

/**
 * This base class represents a sink for audio output
 * to the currently selected output device, at a controllable bitrate.
 *
 * Sinks pull data from a producer.
 */
class audio_sink : public audio_stream
{
    typedef audio_stream super;

    audio_sink* producer_{nullptr};

protected:
    inline audio_sink* producer() const { return producer_; }

public:
    audio_sink() = default;
    audio_sink(int framesize, double samplerate, int channels = 1);

    inline void set_producer(audio_sink* as) {
        producer_ = as;
    }

    /**
     * Produce a frame of audio data to be sent to the sound hardware.
     * This method is typically called from a dedicated audio thread,
     * so the subclass must handle multithread synchronization!
     */
    virtual void produce_output(void* buffer);

    // Overrides to pass the request up the producer chain.

    void set_enabled(bool enable) override;
    void set_frame_size(int frame_size) override;
    void set_sample_rate(double rate) override;
    void set_num_channels(int num_channels) override;

// private: old AbstractAudioOutput had this:
    /**
     * Get output from client, resampling and/or buffering it as necessary
     * to match hwrate and hwframesize (in audio.cc).
     */
    // void get_output(float *buf);
};

} // voicebox namespace
