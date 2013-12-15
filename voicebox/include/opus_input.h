#pragma once

#include "packetized_input.h"
#include "opus.h"

/**
 * This class represents an Opus-encoded source of audio input,
 * providing automatic queueing and interthread synchronization.
 */
class opus_input : public packetized_input
{
    typedef packetized_input super;

    /**
     * Encoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusEncoder *encstate{nullptr};

public:
    opus_input() = default;

    void set_enabled(bool enabling) override;

    byte_array read_frame();

private:
    /**
     * Our implementation of AbstractAudioInput::acceptInput().
     * @param buf [description]
     */
    void accept_input(byte_array) override;
};

