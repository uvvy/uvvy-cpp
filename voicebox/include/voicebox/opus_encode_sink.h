//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "voicebox/audio_sink.h"
#include "opus.h"

namespace voicebox {

/**
 * This class represents an OPUS-encoded sink for audio.
 */
class opus_encode_sink : public audio_sink
{
    using super = audio_sink;

    /**
     * Encoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusEncoder *encode_state_{nullptr};

public:
    opus_encode_sink() = default;

    void set_enabled(bool enabling) override;

private:
    void produce_output(byte_array& buffer) override;
};

} // voicebox namespace
