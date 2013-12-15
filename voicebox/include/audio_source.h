#pragma once

#include "audio_stream.h"
#include "byte_array.h"

namespace voicebox {

/**
 * Sources push data to an acceptor.
 */
class audio_source : public audio_stream
{
    typedef audio_stream super;

    audio_source* acceptor_{nullptr};

protected:
    inline audio_source* acceptor() const { return acceptor_; }

public:
    audio_source() = default;

    inline void set_acceptor(audio_source* as) {
        acceptor_ = as;
    }
    /**
     * Accept input wrapped into a byte_array.
     * The parameters of data inside the array come from audio_stream's settings for
     * frame_size(), num_channels() and sample_rate().
     */
    virtual void accept_input(byte_array data);

    // Overrides to pass the request down the consumer chain.

    void set_enabled(bool enable) override;
    void set_frame_size(int frame_size) override;
    void set_sample_rate(double rate) override;
    void set_num_channels(int num_channels) override;
};

}
