//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <QUdpSocket>
#include <stdint.h>

class UPnPRouter;
namespace bt { class UPnPMCastSocket; }

class UdpTestSender : public QUdpSocket
{
	Q_OBJECT
	UPnPRouter* router;
	bt::UPnPMCastSocket* upnp;

public:
	UdpTestSender(bt::UPnPMCastSocket* upnp);
	~UdpTestSender();

	/**
	 * Construct a packet with return address and send it to @c remote.
	 */
	void ping(QHostAddress remote, uint16_t port);

public slots:
	void routerFound(UPnPRouter*);
	void portForwarded(bool);
	void routerStateChanged();

private slots:
	void onReadyRead();
	void error(QAbstractSocket::SocketError err);
};
