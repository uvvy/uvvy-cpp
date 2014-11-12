//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "sss/stream.h"
#include "voicebox/audio_source.h"

namespace voicebox {

/**
 * Packet source receives data from the network using an sss::stream.
 */
class packet_source : public audio_source
{
    using super = audio_source;

    std::shared_ptr<sss::stream> stream_;
    boost::signals2::connection ready_read_conn;

public:
    packet_source() = default;
    packet_source(std::shared_ptr<sss::stream> stream);
    ~packet_source();

    void set_enabled(bool enable) override;

    void set_source(std::shared_ptr<sss::stream> stream);

private:
    void on_packet_received();
};

} // voicebox namespace
