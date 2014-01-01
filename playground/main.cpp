//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QCoreApplication>
#include "udpsend.h"
#include "upnp/upnpmcastsocket.h"
#include "upnp/router.h"

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    bt::UPnPMCastSocket* upnp = new bt::UPnPMCastSocket(true);
    UdpTestSender sock(upnp);
    QObject::connect(upnp, SIGNAL(discovered(UPnPRouter*)),
        &sock, SLOT(routerFound(UPnPRouter*)));

    // upnp->loadRouters("routers.txt");
    upnp->discover();

    return app.exec();
}
