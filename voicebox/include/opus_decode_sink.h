#pragma once

#include "audio_sink.h"
#include "opus.h"

namespace voicebox {

/**
 * This class represents a high-level sink for audio output by decoding OPUS stream.
 */
class opus_decode_sink : public audio_sink
{
    typedef audio_sink super;

    /**
     * Decoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusDecoder* decstate{nullptr};

public:
    opus_decode_sink() = default;

    void set_enabled(bool enabling) override;

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf [description]
     */
    void produce_output(byte_array& buffer) override;
};

} // voicebox namespace
