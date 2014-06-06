//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "ssu/stream.h"
#include "voicebox/audio_sink.h"

namespace voicebox {

/**
 * Packet sink sends data to the network using an ssu::stream.
 */
class packet_sink : public audio_sink
{
    typedef audio_sink super;

    std::shared_ptr<ssu::stream> stream_;
    uint32_t sequence_number_{0};

public:
    packet_sink(std::shared_ptr<ssu::stream> stream, audio_sink* from = nullptr)
        : audio_sink(from)
        , stream_(stream)
    {}

    void produce_output(byte_array& buffer) override;

    /**
     * Run produce loop until it returns an empty buffer.
     */
    void send_packets();
};

} // voicebox namespace
