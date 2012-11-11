#pragma once

#include <QUdpSocket>

class UdpTestSender : public QUdpSocket
{
	Q_OBJECT

public:
	UdpTestSender();
	~UdpTestSender();

	/**
	 * Construct a packet with return address and send it to @c remote.
	 */
	void ping(QHostAddress remote, uint16_t port);

private slots:
	void onReadyRead();
	void error(QAbstractSocket::SocketError err);
};
