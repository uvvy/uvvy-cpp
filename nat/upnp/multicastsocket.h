#pragma once

#include <QSocket>
#include <QByteArray>

class UpnpRouter;

class UpnpMulticastSocket : public QUdpSocket
{
	Q_OBJECT

	UpnpRouter* parseResponse(const QByteArray & arr);
	void joinUPnPMCastGroup(int fd);
	void leaveUPnPMCastGroup(int fd);

public:
	UpnpMulticastSocket();
	~UpnpMulticastSocket();

public slots:
	/**
	 * Try to discover a UPnP device on the network.
	 * A signal will be emitted when a device is found.
	 */
	void discover();

private slots:
	void onReadyRead();
	void error(QAbstractSocket::SocketError err);
	void onXmlFileDownloaded(UpnpRouter* r,bool success);
	
signals:
	/**
	 * Emitted when a router or internet gateway device is detected.
	 * @param router The router
	 */
	void discovered(UpnpRouter* router);
};
