#pragma once

#include "packetized_output.h"

class raw_output : public packetized_output
{
    typedef packetized_output super;

public:
    raw_output();

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf [description]
     */
    void produce_output(float *buf) override;
};

