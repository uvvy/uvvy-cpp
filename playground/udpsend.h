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
