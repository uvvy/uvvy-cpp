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

using namespace std;
using namespace ssu;

shared_ptr<upnp::UpnpIgdClient> traverse_nat(shared_ptr<host> host)
{
    shared_ptr<upnp::UpnpIgdClient> upnp = make_shared<upnp::UpnpIgdClient>();

    logger::info() << "Initialising UPnP";

    auto endpoints = host->active_local_endpoints();
    set<uint16_t> ports;
    set<boost::asio::ip::address> ips;
    for (auto& ep : endpoints) {
        ips.insert(ep.address());
        ports.insert(ep.port());
    }

    logger::debug() << "Need to map " << dec << ports.size() << " ports";

    upnp->InitControlPoint();

    if (upnp->IsAsync()) {
        upnp->SetNewMappingCallback([&](const int &port, const upnp::ProtocolType &protocol) {
            ports.erase(port);
        });
    }

    bool all_added = true;

    set<boost::asio::ip::udp::endpoint> mapped_endpoints;
    list<upnp::PortMappingExt> out_mapping;
    upnp->GetPortMappings(out_mapping);

    map<uint16_t, boost::asio::ip::udp::endpoint> mapping_map;

    set<uint16_t> used_ports;
    for (auto m : out_mapping)
    {
        used_ports.insert(m.external_port);

        endpoint ep(boost::asio::ip::address::from_string(m.internal_host), m.internal_port);

        mapped_endpoints.insert(ep);

        mapping_map.insert(make_pair(m.external_port, ep));
    }

    auto ports_copy = ports;
    for (auto port : ports_copy)
    {
        bool mapping_exists = false;
        if (contains(used_ports, port)) {
            // We have a mapping for this port, is it for the same address as us? (1)
            auto mapping = mapping_map[port];
            for (auto& ep : endpoints) {
                if (ep == mapping) {
                    // YES(1): We're good
                    // remove port from ports
                    logger::debug() << "NAT found existing port mapping for our endpoint - ext port "
                        << port << "->" << mapping;
                    ports.erase(port);
                    mapping_exists = true;
                    break;
                }
            }
        }
        // IP based check - do we have a mapping for our local endpoint? (2)
        // If so, it must map some external port to our internal port.
        for (auto& ep : endpoints)
        {
            if (ep.port() != port) {
                continue;
            }
            for (auto m : mapping_map)
            {
                if (m.second == ep) {
                    // YES(2): We're good
                    // remove port from ports
                    logger::debug() << "NAT found existing port mapping for our endpoint - ext port "
                        << m.first << "->" << m.second;
                    ports.erase(port);
                    mapping_exists = true;
                    break;
                }
            }
            if (mapping_exists) {
                break;
            }
        }
        // NO(1,2): Set up a new mapping on an unused ext_port
        if (!mapping_exists)
        {
            uint16_t ext_port = port;
            while (contains(used_ports, ext_port)) {
                ++ext_port;
            }
            logger::debug() << "NAT creating port mapping for our endpoint port "
                << dec << port << "->" << ext_port;
            all_added &= upnp->AddPortMapping(port, ext_port, upnp::kTcp);
            all_added &= upnp->AddPortMapping(port, ext_port, upnp::kUdp);
        }
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
