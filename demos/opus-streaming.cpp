//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "arsenal/logging.h"
#include "arsenal/settings_provider.h"
#include "arsenal/any_int_cast.h"
#include "ssu/host.h"
#include "voicebox/audio_service.h"
#include "routing/private/regserver_client.h" // @fixme Testing only.
#include "traverse_nat.h"
#include "client_utils.h"

namespace po = boost::program_options;
using namespace std;
using namespace ssu;

/**
 * Get the address to talk to over IPv6 from the command line.
 * Parses [ipv6::]:port or [ipv6] with default port 9660.
 */
int main(int argc, char* argv[])
{
    bool connect_out{false};
    string peer;
    int port = stream_protocol::default_port;
    vector<string> location_hints;
    bool verbose_debug{false};
    bool disable_upnp{false};

    po::options_description desc("Program arguments");
    desc.add_options()
        ("remote,r", po::value<string>(),
            "Peer EID as base32 identifier")
        ("endpoint,e", po::value<vector<string>>(&location_hints),
            "Endpoint location hint (ipv4 or ipv6 address), can be specified multiple times")
        ("port,p", po::value<int>(&port),
            "Run service on this port, connect peer on this port")
        ("verbose,v", po::bool_switch(&verbose_debug),
            "Print verbose output for debug")
        ("noupnp,n", po::bool_switch(&disable_upnp),
            "Disable UPnP mappings")
        ("help",
            "Print this help message");
    po::positional_options_description p;
    p.add("remote", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (!verbose_debug) {
        logger::set_verbosity(logger::verbosity::info);
    } else {
        logger::logging::set_verbosity(199);
    }

    auto settings = settings_provider::instance();

    auto s_port = settings->get("port");
    if (!s_port.empty()) {
        port = any_int_cast<uint16_t>(s_port);
    }

    if (vm.count("port"))
    {
        port = vm["port"].as<int>();
    }

    if (vm.count("remote"))
    {
        peer = vm["remote"].as<string>();
        connect_out = true;
    }

    settings->set("port", port);
    settings->sync();

    // Shared ptr ensures nat is destroyed on exit...
    shared_ptr<host> host(host::create(settings.get(), port));
    shared_ptr<upnp::UpnpIgdClient> nat;

    if (!disable_upnp) {
        nat = traverse_nat(host);
    }

    uia::routing::internal::regserver_client regclient(host.get());
    regclient_set_profile(settings.get(), regclient, host.get());
    regclient_connect_regservers(settings.get(), regclient);

    voicebox::audio_service service(host);
    service.on_session_started.connect([]{
        logger::info() << "Audio session started";
    });
    service.on_session_finished.connect([]{
        logger::info() << "Audio session stopped";
        // @todo Send termination signal to mainloop
    });

    if (connect_out)
    {
        service.establish_outgoing_session(peer, location_hints);
    }
    else
    {
        logger::info() << "Host " << host->host_identity().id()
            << " listening on port " << dec << port;
        service.listen_incoming_session();
    }

    host->run_io_service();
}
