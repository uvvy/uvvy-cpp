#pragma once

#include "audio_source.h"

namespace voicebox {

/**
 * RtAudio source uses audio_hardware to receive captured input stream.
 */
class rtaudio_source : public audio_source
{
    typedef audio_source super;

public:
    rtaudio_source() = default;

    void set_enabled(bool enable) override;
};

} // voicebox namespace
