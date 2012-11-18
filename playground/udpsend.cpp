#include "udpsend.h"
#include "upnp/router.h"
#include "upnp/upnpmcastsocket.h"

#define LOCAL_PORT 9660

UdpTestSender::UdpTestSender(bt::UPnPMCastSocket* up)
    : QUdpSocket()
    , router(0)
    , upnp(up)
{
    QObject::connect(this,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    QObject::connect(this,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));

    if (!bind(LOCAL_PORT, QUdpSocket::ShareAddress))
        qWarning() << "Cannot bind to UDP port" << LOCAL_PORT;
}

UdpTestSender::~UdpTestSender()
{

}

void UdpTestSender::ping(QHostAddress remote, uint16_t port)
{
    QByteArray b("hello world!");
    qDebug() << "Sending:" << QString(b) << "to" << remote << port;
    writeDatagram(b, remote, port);
}

void UdpTestSender::onReadyRead()
{
    QHostAddress origin;
    uint16_t port;
    QByteArray data(pendingDatagramSize(),0);
    if (readDatagram(data.data(),pendingDatagramSize(),&origin,&port) == -1)
        return;
    
    qDebug() << "Received:" << QString(data);

    ping(origin, port);
}

void UdpTestSender::error(QAbstractSocket::SocketError err)
{
    qWarning() << "Socket error" << err;
}

void UdpTestSender::routerFound(UPnPRouter* r)
{
    qDebug() << "Router detected, punching a hole.";
    // upnp->saveRouters("routers.txt");
    router = r;
    connect(router, SIGNAL(stateChanged()),
        this, SLOT(routerStateChanged()));
    Port p(LOCAL_PORT, Port::TCP);
    router->forward(p);
    Port p2(LOCAL_PORT, Port::UDP);
    router->forward(p2);
}

void UdpTestSender::routerStateChanged()
{
    QString err = router->getError();
    if (err != QString::null)
    {
        qWarning() << "Routing setup error" << err;
        qWarning() << "Giving up";
        exit(1);
    }
    else
    {
        qDebug() << "Router state changed, sending UDP.";
        ping(QHostAddress("212.7.7.70"), LOCAL_PORT);
    }
}
