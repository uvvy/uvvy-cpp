#pragma once

#include "audio_sink.h"

namespace voicebox {

/**
 * RtAudio sink uses audio_hardware to playback received data.
 */
class rtaudio_sink : public audio_sink
{
    typedef audio_sink super;

public:
    rtaudio_sink() = default;

    void set_enabled(bool enable) override;
};

} // voicebox namespace
