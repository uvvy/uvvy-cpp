//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <queue>
#include <mutex>
#include <boost/asio/strand.hpp>
#include "audio_service.h"
#include "audio_hardware.h"
#include "plotfile.h"
#include "stream.h"
#include "server.h"
#include "logging.h"
#include "algorithm.h"

#include "rtaudio_source.h"
#include "rtaudio_sink.h"
#include "packet_source.h"
#include "packet_sink.h"
#include "packetizer.h"
#include "jitterbuffer.h"
#include "opus_encode_sink.h"
#include "opus_decode_sink.h"

using namespace std;
using namespace ssu;
using namespace voicebox;
using namespace boost::asio;

static const std::string service_name{"streaming"};
static const std::string protocol_name{"opus"};

struct send_chain
{
    rtaudio_source hw_in_;
    packetizer pkt_;
    opus_encode_sink encoder_;
    packet_sink sink_; // maintains seqno
    io_service::strand strand_;

    send_chain(shared_ptr<stream> stream)
        : sink_(stream)
        , strand_(stream->get_host()->get_io_service())
    {
        hw_in_.set_acceptor(&pkt_);
        sink_.set_producer(&encoder_);
        encoder_.set_producer(&pkt_);

        pkt_.on_ready_read.connect([this] { strand_.post([this] { sink_.send_packets(); }); });
    }

    void enable()
    {
        sink_.enable();
        hw_in_.enable();
    }

    void disable()
    {
        hw_in_.disable();
        sink_.disable();
    }
};

struct receive_chain
{
    packet_source source_; // maintains seqno
    jitterbuffer jb_;
    opus_decode_sink decoder_;
    rtaudio_sink hw_out_;

    receive_chain(shared_ptr<stream> stream)
        : source_(stream)
    {
        source_.set_acceptor(&jb_);
        hw_out_.set_producer(&decoder_);
        decoder_.set_producer(&jb_);
    }

    void enable()
    {
        source_.enable();
    }

    void disable()
    {
        source_.disable();
        hw_out_.disable();
    }
};

//=================================================================================================
// audio_service
//=================================================================================================

class audio_service::private_impl
{
public:
    std::shared_ptr<ssu::host> host_;
    shared_ptr<stream> stream;
    shared_ptr<server> server;
    shared_ptr<send_chain> send;
    shared_ptr<receive_chain> recv;

    private_impl(std::shared_ptr<ssu::host> host)
        : host_(host)
    {}
};

audio_service::audio_service(std::shared_ptr<ssu::host> host)
    : pimpl_(make_shared<private_impl>(host))
{}

audio_service::~audio_service()
{}

void audio_service::establish_outgoing_session(peer_id const& eid,
    std::vector<std::string> ep_hints)
{
    logger::info() << "Connecting to " << eid;

    pimpl_->stream = make_shared<ssu::stream>(pimpl_->host_);

    pimpl_->send = make_shared<send_chain>(pimpl_->stream);

    pimpl_->stream->on_link_up.connect([this] {
        on_session_started();
        pimpl_->send->enable();
        // voicebox::audio_hardware::instance()->start_audio();
    });
    pimpl_->stream->on_link_down.connect([this] {
        on_session_finished();
        pimpl_->send->disable();
        // voicebox::audio_hardware::instance()->stop_audio();
    });
    pimpl_->stream->connect_to(eid, service_name, protocol_name);

    if (!ep_hints.empty())
    {
        for (auto epstr : ep_hints)
        {
            // @todo Allow specifying a port too.
            // @todo Allow specifying a DNS name for endpoint.
            ssu::endpoint ep(boost::asio::ip::address::from_string(epstr),
                stream_protocol::default_port);
            logger::debug() << "Connecting at location hint " << ep;
            pimpl_->stream->connect_at(ep);
        }
    }
}

void audio_service::listen_incoming_session()
{
    pimpl_->server = make_shared<ssu::server>(pimpl_->host_);
    pimpl_->server->on_new_connection.connect([this]
    {
        new_connection(pimpl_->server);
    });
    bool listening = pimpl_->server->listen(service_name, "Streaming services",
                                            protocol_name, "OPUS Audio protocol");
    assert(listening);
    if (!listening) {
        throw runtime_error("Couldn't set up server listening to streaming:opus");
    }
}


void audio_service::new_connection(shared_ptr<server> server)
{
    auto stream = server->accept();
    if (!stream) {
        return;
    }

    logger::info() << "New incoming connection from " << stream->remote_host_id();
    // streaming(stream);

    pimpl_->recv = make_shared<receive_chain>(stream);

    stream->on_link_up.connect([this] {
        on_session_started();
        pimpl_->recv->enable();
    });

    stream->on_link_down.connect([this] {
        pimpl_->recv->disable();
        on_session_finished();
    });

    if (stream->is_link_up())
    {
        logger::debug() << "Incoming stream is ready, start sending immediately.";
        on_session_started();
        pimpl_->recv->enable();
    }
}
