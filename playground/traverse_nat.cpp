//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "traverse_nat.h"
#include "logging.h"
#include "host.h"
// #include "private/regserver_client.h" // @fixme Testing only.

using namespace std;
using namespace ssu;

// constexpr uint16_t regserver_port = uia::routing::internal::REGSERVER_DEFAULT_PORT;

shared_ptr<upnp::UpnpIgdClient> traverse_nat(std::shared_ptr<host> host)
{
    shared_ptr<upnp::UpnpIgdClient> upnp = make_shared<upnp::UpnpIgdClient>();

    logger::info() << "Initialising UPnP";

    auto endpoints = host->active_local_endpoints();
    std::set<uint16_t> ports;
    for (auto& ep : endpoints) {
        ports.insert(ep.port());
    }

    logger::debug() << "Need to map " << ports.size() << " ports";

    upnp->InitControlPoint();

    if (upnp->IsAsync()) {
        upnp->SetNewMappingCallback([&](const int &port, const upnp::ProtocolType &protocol) {
            ports.erase(port);
        });
    }

    bool all_added = true;
    for (uint16_t port : ports)
    {
        all_added &= upnp->AddPortMapping(port, upnp::kTcp);
        all_added &= upnp->AddPortMapping(port, upnp::kUdp);
    }

    if (upnp->IsAsync()) {
        logger::debug() << "Waiting...";
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }

    if (upnp->HasServices()) {
        logger::debug() << "External IP: " << upnp->GetExternalIpAddress();
        assert(all_added);
        if (upnp->IsAsync()) {
            assert(ports.size() == 0);
        }
        logger::info() << "All UPnP mappings successful";
    } else {
        logger::warning() << "Sorry, no port mappings via UPnP possible";
    }

    return upnp;
}
