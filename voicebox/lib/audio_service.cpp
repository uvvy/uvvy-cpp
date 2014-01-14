//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <queue>
#include <mutex>
#include <boost/asio/strand.hpp>
#include "voicebox/audio_service.h"
#include "voicebox/audio_hardware.h"
#include "ssu/stream.h"
#include "ssu/server.h"
#include "logging.h"
#include "algorithm.h"

#include "voicebox/rtaudio_source.h"
#include "voicebox/rtaudio_sink.h"
#include "voicebox/packet_source.h"
#include "voicebox/packet_sink.h"
#include "voicebox/packetizer.h"
#include "voicebox/jitterbuffer.h"
#include "voicebox/opus_encode_sink.h"
#include "voicebox/opus_decode_sink.h"

using namespace std;
using namespace ssu;
using namespace voicebox;
using namespace boost::asio;

namespace {

const string service_name{"streaming"};
const string protocol_name{"opus"};
const uint32_t cmd_magic = 0x61434d44; // aCMD
const int cmd_start_session = 1001;
const int cmd_stop_session = 1002;

//=================================================================================================
// send_chain
//=================================================================================================

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
        logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__;
        sink_.enable();
        hw_in_.enable();
    }

    void disable()
    {
        logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__;
        hw_in_.disable();
        sink_.disable();
    }
};

//=================================================================================================
// receive_chain
//=================================================================================================

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
        logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__;
        source_.enable();
        hw_out_.enable();
    }

    void disable()
    {
        logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__;
        source_.disable();
        hw_out_.disable();
    }
};

} // anonymous namespace

//=================================================================================================
// audio_service::private_impl
//=================================================================================================

namespace voicebox {

class audio_service::private_impl
{
    audio_service* parent_{0};
    shared_ptr<host> host_;
    shared_ptr<stream> stream_;
    shared_ptr<server> server_;
    shared_ptr<send_chain> send_;
    shared_ptr<receive_chain> recv_;
    bool active_{false};

public:
    private_impl(audio_service* parent, shared_ptr<ssu::host> host)
        : parent_(parent)
        , host_(host)
    {}

    inline bool is_active() const { return active_; }

    void send_command(int cmd)
    {
        byte_array msg;
        {
            byte_array_owrap<flurry::oarchive> write(msg);
            write.archive() << cmd_magic << cmd;
        }
        stream_->write_record(msg);
    }

    void session_command_handler(byte_array const& msg);
    void connect_stream(shared_ptr<stream> stream);
    void new_connection(shared_ptr<server> server);

    void establish_outgoing_session(peer_id const& eid, vector<string> ep_hints);
    void listen_incoming_session();
    void end_session();
};

// When start session command is received, start sending
// When stop session command is received, stop sending and reply with stop_session (unless stopped)
void audio_service::private_impl::session_command_handler(byte_array const& msg)
{
    uint32_t magic;
    int cmd;
    byte_array_iwrap<flurry::iarchive> read(msg);
    read.archive() >> magic >> cmd;

    switch (cmd)
    {
        case cmd_start_session:
            if (!active_)
            {
                logger::debug(TRACE_DETAIL) << "cmd_start_session, starting audio session";
                send_->enable();
                recv_->enable();
                active_ = true;
                parent_->on_session_started();
            }
            break;
        case cmd_stop_session:
            logger::debug(TRACE_DETAIL) << "cmd_stop_session, stopping audio session";
            end_session();
            break;
        default:
            break;
    }
}

void audio_service::private_impl::connect_stream(shared_ptr<stream> stream)
{
    // Handle session commands
    stream->on_ready_read_record.connect([=] {
        byte_array msg = stream->read_record();
        logger::debug(TRACE_DETAIL) << "Received audio service command: " << msg;
        session_command_handler(msg);
    });

    stream->on_link_up.connect([this] {
        logger::debug(TRACE_DETAIL) << "Link up, sending start session command";
        send_command(cmd_start_session);
    });

    stream->on_link_down.connect([this] {
        logger::debug(TRACE_DETAIL) << "Link down, stopping session and disabling audio";
        parent_->end_session();
    });

    if (stream->is_link_up())
    {
        logger::debug(TRACE_DETAIL) << "Incoming stream is ready, start sending immediately.";
        send_command(cmd_start_session);
    }
}

void audio_service::private_impl::establish_outgoing_session(peer_id const& eid,
                                                             vector<string> ep_hints)
{
    stream_ = make_shared<ssu::stream>(host_);

    send_ = make_shared<send_chain>(stream_);
    recv_ = make_shared<receive_chain>(stream_);

    connect_stream(stream_);

    stream_->connect_to(eid, service_name, protocol_name);

    if (!ep_hints.empty())
    {
        for (auto epstr : ep_hints)
        {
            // @todo Allow specifying a port too.
            // @todo Allow specifying a DNS name for endpoint.
            ssu::endpoint ep(boost::asio::ip::address::from_string(epstr),
                stream_protocol::default_port);
            logger::debug(TRACE_DETAIL) << "Connecting at location hint " << ep;
            stream_->connect_at(ep);
        }
    }
}

void audio_service::private_impl::new_connection(shared_ptr<server> server)
{
    auto stream = server->accept();
    if (!stream) {
        return;
    }
    assert(!stream_);
    stream_ = stream;

    logger::info() << "New incoming connection from " << stream_->remote_host_id();

    recv_ = make_shared<receive_chain>(stream_);
    if (!send_) {
        send_ = make_shared<send_chain>(stream_);
    }

    connect_stream(stream_);
}

void audio_service::private_impl::listen_incoming_session()
{
    server_ = make_shared<ssu::server>(host_);
    server_->on_new_connection.connect([this]
    {
        new_connection(server_);
    });
    bool listening = server_->listen(service_name, "Streaming services",
                                     protocol_name, "OPUS Audio protocol");
    assert(listening);
    if (!listening) {
        throw runtime_error("Couldn't set up server listening to streaming:opus");
    }
}

// @todo After ending a session you cannot start it again.
void audio_service::private_impl::end_session()
{
    send_command(cmd_stop_session);
    if (send_)
    {
        send_->disable();
        send_ = nullptr;
    }
    if (recv_)
    {
        recv_->disable();
        recv_ = nullptr;
    }
    active_ = false;
    parent_->on_session_finished();
    //disconnect stream here
    stream_ = nullptr;
}

//=================================================================================================
// audio_service
//=================================================================================================

audio_service::audio_service(shared_ptr<ssu::host> host)
    : pimpl_(make_shared<private_impl>(this, host))
{}

audio_service::~audio_service()
{}

bool audio_service::is_active() const
{
    return pimpl_->is_active();
}

void audio_service::establish_outgoing_session(peer_id const& eid,
                                               vector<string> ep_hints)
{
    logger::info() << "Connecting to " << eid;
    pimpl_->establish_outgoing_session(eid, ep_hints);
}

void audio_service::listen_incoming_session()
{
    pimpl_->listen_incoming_session();
}

// Disconnected or "STOP SESSION" command received.
void audio_service::end_session()
{
    pimpl_->end_session();
}

} // voicebox namespace

