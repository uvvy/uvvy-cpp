//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "udpsend.h"
#include "upnp/router.h"
#include "upnp/upnpmcastsocket.h"
#include "protocol.h"
#include <iostream>

UdpTestSender::UdpTestSender(bt::UPnPMCastSocket* up)
    : QUdpSocket()
    , router(0)
    , upnp(up)
{
    QObject::connect(this,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    QObject::connect(this,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));

    if (!bind(stream_protocol::default_port, QUdpSocket::ShareAddress))
        qWarning() << "Cannot bind to UDP port" << stream_protocol::default_port;

    ping(QHostAddress("127.0.0.1"), stream_protocol::default_port);
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
    connect(router, SIGNAL(portForwarded(bool)),
        this, SLOT(portForwarded(bool)));

    Port p2(stream_protocol::default_port, Port::UDP);
    router->forward(p2, /*leaseDuration:*/ 3600, /*extPort:*/ 0);
}

void UdpTestSender::portForwarded(bool success)
{
    if (success)
    {
        qDebug() << "Port forwarding succeeded, sending UDP.";
        ping(QHostAddress("212.7.7.70"), stream_protocol::default_port);
    }
}

void UdpTestSender::routerStateChanged()
{
    QString err = router->getError();
    if (err != QString::null)
    {
        qWarning() << "Routing setup error" << err;
        qWarning() << "Giving up";
        // exit(1);
    }
}
