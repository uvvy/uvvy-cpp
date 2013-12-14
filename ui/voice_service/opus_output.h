#pragma once

#include "packetized_output.h"
#include "opus.h"

/**
 * This class represents a high-level sink for audio output by decoding OPUS stream.
 */
class opus_output : public packetized_output
{
    typedef packetized_output super;

    /**
     * Decoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusDecoder* decstate{nullptr};

public:
    opus_output() = default;

    void set_enabled(bool enabling) override;

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf [description]
     */
    void produce_output(float *buf) override;
};

