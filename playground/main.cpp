#include <QCoreApplication>
#include "udpsend.h"
#include "upnp/upnpmcastsocket.h"
#include "upnp/upnprouter.h"

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

    bt::UPnPMCastSocket* upnp = new bt::UPnPMCastSocket(true);
    UdpTestSender sock(upnp);
    QObject::connect(upnp, SIGNAL(discovered(bt::UPnPRouter*)),
        &sock, SLOT(routerFound(bt::UPnPRouter*)));

    upnp->loadRouters("routers.txt");

	return app.exec();
}
