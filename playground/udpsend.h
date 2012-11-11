#pragma once

#include <QUdpSocket>
#include <stdint.h>

namespace bt { class UPnPRouter; class UPnPMCastSocket; }

class UdpTestSender : public QUdpSocket
{
	Q_OBJECT
	bt::UPnPRouter* router;
	bt::UPnPMCastSocket* upnp;

public:
	UdpTestSender(bt::UPnPMCastSocket* upnp);
	~UdpTestSender();

	/**
	 * Construct a packet with return address and send it to @c remote.
	 */
	void ping(QHostAddress remote, uint16_t port);

public slots:
	void routerFound(bt::UPnPRouter*);
	void routerStateChanged();

private slots:
	void onReadyRead();
	void error(QAbstractSocket::SocketError err);
};
