//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "stream.h"
#include "audio_sink.h"

namespace voicebox {

/**
 * Packet sink sends data to the network using an ssu::stream.
 */
class packet_sink : public audio_sink
{
    std::shared_ptr<ssu::stream> stream_;

public:
    packet_sink(std::shared_ptr<ssu::stream> stream)
        : audio_sink()
        , stream_(stream)
    {}

    void produce_output(byte_array& buffer) override;
};

} // voicebox namespace
