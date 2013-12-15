#pragma once

#include "audio_sink.h"
#include "opus.h"

/**
 * This class represents an OPUS-encoded sink for audio.
 */
class opus_encode_sink : public audio_sink
{
    typedef audio_sink super;

    /**
     * Encoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusEncoder *encstate{nullptr};

public:
    opus_encode_sink() = default;

    void set_enabled(bool enabling) override;

private:
    void produce_output(byte_array& buffer) override;
};
