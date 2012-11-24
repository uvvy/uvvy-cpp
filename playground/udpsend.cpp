#include "udpsend.h"
#include "upnp/router.h"
#include "upnp/upnpmcastsocket.h"

#include <iostream>

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
    upnp->saveRouters("routers.txt");
    router = r;
    connect(router, SIGNAL(stateChanged()),
        this, SLOT(routerStateChanged()));

    Port p2(LOCAL_PORT, Port::UDP);
    // router->undoForward(p2);
    router->forward(p2, /*leaseDuration:*/ 3600, /*extPort:*/ 0/*p2.number*/);

    ping(QHostAddress("212.7.7.70"), LOCAL_PORT);
    std::string text;
    std::cin >> text;
    ping(QHostAddress("212.7.7.70"), LOCAL_PORT);

    // QList<UPnPRouter::ForwardingEntry> list;
    // router->getPortForwardings(list);
}

void UdpTestSender::routerStateChanged()
{
    ping(QHostAddress("212.7.7.70"), LOCAL_PORT);

    QString err = router->getError();
    if (err != QString::null)
    {
        qWarning() << "Routing setup error" << err;
        qWarning() << "Giving up";
        // exit(1);
    }
    else
    {
        qDebug() << "Router state changed, sending UDP.";
        ping(QHostAddress("212.7.7.70"), LOCAL_PORT);
    }
}
