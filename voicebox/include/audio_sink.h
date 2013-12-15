#pragma once

#include "audio_stream.h"

namespace voicebox {

/**
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

    inline void set_producer(audio_sink* as) {
        producer_ = as;
    }
    virtual void produce_output(void* buffer);

    // Overrides to pass the request up the producer chain.

    void set_enabled(bool enable) override;
    void set_frame_size(int frame_size) override;
    void set_sample_rate(double rate) override;
    void set_num_channels(int num_channels) override;
};

}
