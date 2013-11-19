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
#include "private/regserver_client.h" // @fixme Testing only.

using namespace std;

constexpr uint16_t regserver_port = uia::routing::internal::REGSERVER_DEFAULT_PORT;

shared_ptr<upnp::UpnpIgdClient> traverse_nat(int main_port)
{
    shared_ptr<upnp::UpnpIgdClient> upnp = make_shared<upnp::UpnpIgdClient>();

    logger::info() << "Initialising UPnP";

    upnp->InitControlPoint();

    bool main_port_mapped{false}, regserver_port_mapped{false};

    if (upnp->IsAsync()) {
        upnp->SetNewMappingCallback([&](const int &port, const upnp::ProtocolType &protocol) {
            if (port == main_port) {
                main_port_mapped = true;
            } else if (port == regserver_port) {
                regserver_port_mapped = true;
            }
        });
    }

    bool all_added = true;
    all_added &= upnp->AddPortMapping(main_port, upnp::kTcp);
    all_added &= upnp->AddPortMapping(main_port, upnp::kUdp);
    all_added &= upnp->AddPortMapping(regserver_port, upnp::kTcp);
    all_added &= upnp->AddPortMapping(regserver_port, upnp::kUdp);

    if (upnp->IsAsync()) {
        logger::debug() << "Waiting...";
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }

    if (upnp->HasServices()) {
      logger::debug() << "External IP: " << upnp->GetExternalIpAddress();
      assert(all_added);
      if (upnp->IsAsync()) {
        assert(main_port_mapped and regserver_port_mapped);
      }
      logger::info() << "All UPnP mappings successful";
    } else {
      logger::warning() << "Sorry, no port mappings via UPnP possible";
    }

    return upnp;
}
