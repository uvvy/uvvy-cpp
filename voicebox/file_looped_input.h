#pragma once

#include <fstream>
#include "packetized_input.h"

/**
 * This class provides data from pseudo-input by reading a specified file in a loop.
 * It provides network streaming of a given file.
 *
 * Current implementation reads mono 16 bit raw audio file in an endless loop.
 */
class file_looped_input : public packetized_input
{
    typedef packetized_input super;

    std::ifstream file;
    int64_t offset;

public:
    file_looped_input(const QString& fileName);

    void set_enabled(bool enabling) override;

    /**
     * Disable the stream and clear the output queue.
     */
    void reset();

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf Output hardware buffer to write data to. The size is hwframesize.
     */
    void produce_output(float *buf) override;
};
