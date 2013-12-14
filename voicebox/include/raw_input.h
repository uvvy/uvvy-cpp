#pragma once

#include "packetized_input.h"

/**
 * This class represents a raw unencoded source of audio input,
 * providing automatic queueing and interthread synchronization.
 * This data is usually received from network, but could also be supplied from a file.
 * It uses an XDR-based encoding to simplify interoperation.
 */
class raw_input : public packetized_input
{
    typedef packetized_input super;

public:
    raw_input();

private:
    /**
     * Our implementation of AbstractAudioInput::acceptInput().
     * @param buf [description]
     */
    void accept_input(const float *buf) override;
};

