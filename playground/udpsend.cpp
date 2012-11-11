#include "udpsend.h"

UdpTestSender::UdpTestSender()
	: QUdpSocket()
{
	QObject::connect(this,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
	QObject::connect(this,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));

	if (!bind(9660, QUdpSocket::ShareAddress))
		qWarning() << "Cannot bind to UDP port 9660";
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

}
