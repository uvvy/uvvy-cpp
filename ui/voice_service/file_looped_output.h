#pragma once

#include <fstream>
#include "packetized_output.h"

/**
 * This class provides data to hardware output by reading a specified file in a loop.
 * It provides a local-only playback of a given file.
 * It can be used by signaling on both sides to play a call signal, for example.
 * If you want to send the file contents over the wire, use FileLoopedInput.
 *
 * Current implementation plays mono 16 bit raw audio file in an endless loop.
 */
class file_looped_output : public packetized_output
{
    typedef packetized_output super;

    std::string filename_;
    std::ifstream file_;
    size_t offset_{0};

public:
    file_looped_output(std::string const& filename);

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
