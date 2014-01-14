//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/any.hpp>
#include "settings_provider.h"
#include "ssu/host.h"
#include "client_profile.h"
#include "client_utils.h"
#include "private/regserver_client.h"

using namespace std;

void regclient_set_profile(settings_provider* settings,
    uia::routing::internal::regserver_client& regclient,
    ssu::host* host)
{
    // Pull client profile from settings.
    byte_array s_client = settings->get_byte_array("profile");
    uia::routing::client_profile client(s_client);
    client.set_endpoints(set_to_vector(host->active_local_endpoints()));
    for (auto kw : client.keywords()) {
        logger::debug() << "Keyword: " << kw;
    }
    regclient.set_profile(client);
}

void regclient_connect_regservers(settings_provider* settings,
    uia::routing::internal::regserver_client& regclient)
{
    byte_array s_rs = settings->get_byte_array("regservers");
    if (!s_rs.is_empty())
    {
        byte_array rs_ba(s_rs);
        byte_array_iwrap<flurry::iarchive> read(rs_ba);
        vector<string> regservers;
        read.archive() >> regservers;
        for (auto server : regservers)
        {
            regclient.register_at(server);
        }
    }
}
