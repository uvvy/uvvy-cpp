#pragma once

#include <fstream>
#include "voicebox/audio_sink.h"

namespace voicebox {

/**
 * This class provides data from pseudo-input by reading a specified file in a loop.
 * Current implementation reads 16 bit raw audio file in an endless loop. Set sample rate
 * and number of channels through a regular audio_stream interface.
 */
class file_read_sink : public audio_sink
{
    using super = audio_sink;

    std::string filename_;
    std::ifstream file_;
    size_t offset_{0};

public:
    file_read_sink(std::string const& filename);

    void set_enabled(bool enabling) override;

    void produce_output(byte_array& buffer) override;
};

} // voicebox namespace
