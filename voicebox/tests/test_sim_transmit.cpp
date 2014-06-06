//
// Capture audio locally,
// Send it through simulated network,
// Playback locally again.
//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_TEST_MODULE Test_sim_transmit
#include <boost/test/unit_test.hpp>

#include "../../ssu/tests/simulator_fixture.h" //@todo Fix include directory for test fixtures
#include "voicebox/rtaudio_source.h"
#include "voicebox/rtaudio_sink.h"
#include "voicebox/packetizer.h"
#include "voicebox/jitterbuffer.h"
#include "voicebox/packet_source.h"
#include "voicebox/packet_sink.h"

using namespace std;
using namespace ssu;
using namespace ssu::simulation;
using namespace voicebox;

BOOST_FIXTURE_TEST_CASE(simple_sim_step, simulator_fixture)
{
    // source_->pkt_->sender_
    rtaudio_source source_;
    packetizer pkt_(&source_);
    packet_sink sender_(client, &pkt_);
    pkt_.on_ready_read.connect([&] {
        logger::debug() << "Packetizer on_ready_read - posting send event";
        simulator->post([&] {
            logger::debug() << "SEND EVENT - sending all pending packets";
            sender_.send_packets();
        });
    });

    // receiver_->jb_->output_
    packet_source receiver_;
    jitterbuffer jb_(&receiver_);
    rtaudio_sink output_(&jb_);

    client->on_link_up.connect([&] {
        logger::debug() << "client: link_up";
        sender_.enable();
        source_.enable();
    });

    server->on_new_connection.connect([&] {
        logger::debug() << "server: on_new_connection";
        auto stream = server->accept();
        if (!stream) {
            return;
        }
        receiver_.set_source(stream);
        receiver_.enable();
        output_.enable();
    });

    logger::logging::set_verbosity(255);

    client->connect_to(server_host_eid, "simulator", "test", server_host_address);

    simulator->run();

    this_thread::sleep_for(chrono::milliseconds(20));

    simulator->run();
}
