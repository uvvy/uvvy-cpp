//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "audio_sink.h"
#include "opus.h"

namespace voicebox {

/**
 * This class represents an OPUS-encoded sink for audio.
 */
class opus_encode_sink : public audio_sink
{
    typedef audio_sink super;

    /**
     * Encoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusEncoder *encstate{nullptr};

public:
    opus_encode_sink() = default;

    void set_enabled(bool enabling) override;

private:
    void produce_output(byte_array& buffer) override;
};

} // voicebox namespace