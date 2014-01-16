//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "ssu/stream.h"
#include "voicebox/audio_source.h"

namespace voicebox {

/**
 * Packet source receives data from the network using an ssu::stream.
 */
class packet_source : public audio_source
{
    typedef audio_source super;

    std::shared_ptr<ssu::stream> stream_;
    boost::signals2::connection ready_read_conn;

public:
    packet_source() = default;
    packet_source(std::shared_ptr<ssu::stream> stream);
    ~packet_source();

    void set_enabled(bool enable) override;

    void set_source(std::shared_ptr<ssu::stream> stream);

private:
    void on_packet_received();
};

} // voicebox namespace
