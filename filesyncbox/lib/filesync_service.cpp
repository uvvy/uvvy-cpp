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
#include "filesyncbox/filesync_service.h"
#include "stream.h"
#include "server.h"
#include "logging.h"

using namespace std;
using namespace ssu;
using namespace filesyncbox;
using namespace boost::asio;

namespace {

const string service_name{"filesync"};
const string protocol_name{"treesync-v1"};

} // anonymous namespace

namespace filesyncbox {

//=================================================================================================
// filesync_service::private_impl
//=================================================================================================

class filesync_service::private_impl
{
    filesync_service* parent_{0};
    shared_ptr<host> host_;
    shared_ptr<stream> stream_;
    shared_ptr<server> server_;
    bool active_{false};

public:
    private_impl(filesync_service* parent, shared_ptr<ssu::host> host)
        : parent_(parent)
        , host_(host)
    {}

    inline bool is_active() const { return active_; }

    void session_command_handler(byte_array const& msg);
    void connect_stream(shared_ptr<stream> stream);
    void new_connection(shared_ptr<server> server);
    void listen_incoming_session();
};

void filesync_service::private_impl::session_command_handler(byte_array const& msg)
{
    uint32_t magic;
    int cmd;
    byte_array_iwrap<flurry::iarchive> read(msg);
    read.archive() >> magic >> cmd;

    switch (cmd)
    {
    }
}

void filesync_service::private_impl::connect_stream(shared_ptr<stream> stream)
{
    // Handle session commands
    stream->on_ready_read_record.connect([=] {
        byte_array msg = stream->read_record();
        logger::debug() << "Received filesync service command: " << msg;
        session_command_handler(msg);
    });

    stream->on_link_up.connect([this] {
        logger::debug() << "Link up, sending start session command";
        // send_command(cmd_start_session);
    });

    stream->on_link_down.connect([this] {
        logger::debug() << "Link down, stopping session and disabling filesync";
        // end_session();
    });

    if (stream->is_link_up())
    {
        logger::debug() << "Incoming stream is ready, start sync immediately.";
        // send_command(cmd_start_session);
    }
}

void filesync_service::private_impl::new_connection(shared_ptr<server> server)
{
    auto stream = server->accept();
    if (!stream) {
        return;
    }
    assert(!stream_);
    stream_ = stream;

    logger::info() << "New incoming connection from " << stream_->remote_host_id();
    connect_stream(stream_);
}

void filesync_service::private_impl::listen_incoming_session()
{
    server_ = make_shared<ssu::server>(host_);
    server_->on_new_connection.connect([this]
    {
        new_connection(server_);
    });
    bool listening = server_->listen(service_name, "Syncing services",
                                     protocol_name, "File tree sync version 1");
    assert(listening);
    if (!listening) {
        throw runtime_error("Couldn't set up server listening to streaming:opus");
    }
}

//=================================================================================================
// filesync_service
//=================================================================================================

filesync_service::filesync_service(shared_ptr<ssu::host> host)
    : pimpl_(make_shared<private_impl>(this, host))
{
    pimpl_->listen_incoming_session();
}

filesync_service::~filesync_service()
{}

bool filesync_service::is_active() const
{
    return pimpl_->is_active();
}

void add_directory_sync(boost::filesystem::path dir, std::vector<ssu::peer_id> const& to_peers)
{}

} // filesyncbox namespace

