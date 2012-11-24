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
