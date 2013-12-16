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
#include "audio_service.h"
#include "audio_hardware.h"
#include "plotfile.h"
#include "stream.h"
#include "server.h"
#include "logging.h"
#include "algorithm.h"

using namespace std;
using namespace ssu;

static const std::string service_name{"streaming"};
static const std::string protocol_name{"opus"};

//=================================================================================================
// audio_service
//=================================================================================================

class audio_service::private_impl
{
public:
    std::shared_ptr<ssu::host> host_;
    shared_ptr<stream> stream;
    shared_ptr<server> server;

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
    // pimpl_->hw.streaming(pimpl_->stream);
    pimpl_->stream->on_link_up.connect([this] {
        on_session_started();
        voicebox::audio_hardware::instance()->start_audio();
    });
    pimpl_->stream->on_link_down.connect([this] {
        on_session_finished();
        voicebox::audio_hardware::instance()->stop_audio();
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
        // pimpl_->hw.new_connection(pimpl_->server,
        //     [this] { on_session_started(); },
        //     [this] { on_session_finished(); });
    });
    bool listening = pimpl_->server->listen(service_name, "Streaming services",
                                            protocol_name, "OPUS Audio protocol");
    assert(listening);
    if (!listening) {
        throw runtime_error("Couldn't set up server listening to streaming:opus");
    }
}

