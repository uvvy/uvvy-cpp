//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "audio_sink.h"
#include "opus.h"

namespace voicebox {

/**
 * This class represents a high-level sink for audio output by decoding OPUS stream.
 */
class opus_decode_sink : public audio_sink
{
    typedef audio_sink super;

    /**
     * Decoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusDecoder* decode_state_{nullptr};

public:
    opus_decode_sink() = default;

    void set_enabled(bool enabling) override;

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf [description]
     */
    void produce_output(byte_array& buffer) override;
};

} // voicebox namespace
