//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Capture audio locally
// Send it through simulated network
// Playback locally again
//
#define BOOST_TEST_MODULE Test_sim_transmit
#include <boost/test/unit_test.hpp>

#include "stream.h"
#include "server.h"
#include "simulation/simulator.h"
#include "simulation/sim_host.h"
#include "simulation/sim_link.h"
#include "simulation/sim_connection.h"
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

BOOST_AUTO_TEST_CASE(simple_sim_step)
{
    shared_ptr<simulator> sim(make_shared<simulator>());
    BOOST_CHECK(sim != nullptr);

    shared_ptr<sim_host> my_host(sim_host::create(sim));
    BOOST_CHECK(my_host != nullptr);
    endpoint my_host_address(
        boost::asio::ip::address_v4::from_string("10.0.0.1"), stream_protocol::default_port);
    shared_ptr<sim_host> other_host(sim_host::create(sim));
    BOOST_CHECK(other_host != nullptr);
    endpoint other_host_address(
        boost::asio::ip::address_v4::from_string("10.0.0.2"), stream_protocol::default_port);

    shared_ptr<sim_connection> conn = make_shared<sim_connection>();
    BOOST_CHECK(conn != nullptr);
    conn->connect(other_host, other_host_address,
                  my_host, my_host_address);

    shared_ptr<ssu::link> link = my_host->create_link();
    BOOST_CHECK(link != nullptr);
    link->bind(my_host_address);
    BOOST_CHECK(link->is_active());

    shared_ptr<ssu::link> other_link = other_host->create_link();
    BOOST_CHECK(other_link != nullptr);
    other_link->bind(other_host_address);
    BOOST_CHECK(other_link->is_active());

    shared_ptr<ssu::server> other_server(make_shared<ssu::server>(other_host));
    BOOST_CHECK(other_server != nullptr);
    bool listening = other_server->listen("streaming", "Simulating", "opus", "OPUS streaming");
    BOOST_CHECK(listening == true);
    shared_ptr<ssu::stream> sim_out_stream(make_shared<stream>(my_host));
    BOOST_CHECK(sim_out_stream != nullptr);
    bool hinted = sim_out_stream->add_location_hint(
        other_host->host_identity().id(), other_host_address); // no routing yet
    BOOST_CHECK(hinted == true);

    // source_->pkt_->sender_
    rtaudio_source source_;
    packetizer pkt_(&source_);
    packet_sink sender_(sim_out_stream, &pkt_);
    pkt_.on_ready_read.connect([&] {
        logger::debug() << "Packetizer on_ready_read - posting send event";
        sim->post([&] {
            logger::debug() << "SEND EVENT - sending all pending packets";
            sender_.send_packets();
        });
    });

    // receiver_->jb_->output_
    packet_source receiver_;
    jitterbuffer jb_(&receiver_);
    rtaudio_sink output_(&jb_);

    sim_out_stream->on_link_up.connect([&] {
        logger::debug() << "sim_out_stream: link_up";
        sender_.enable();
        source_.enable();
    });

    other_server->on_new_connection.connect([&] {
        logger::debug() << "other_server: on_new_connection";
        auto stream = other_server->accept();
        if (!stream) {
            return;
        }
        receiver_.set_source(stream);
        receiver_.enable();
        output_.enable();
    });

    logger::logging::set_verbosity(255);

    sim_out_stream->connect_to(other_host->host_identity().id(),
        "streaming", "opus", other_host_address);

    sim->run();

    this_thread::sleep_for(chrono::milliseconds(20));

    sim->run();
}
