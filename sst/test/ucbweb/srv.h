/*
 * Structured Stream Transport
 * Copyright (C) 2006-2008 Massachusetts Institute of Technology
 * Author: Bryan Ford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef SRV_H
#define SRV_H

#include <QObject>

class QIODevice;
class QTcpServer;
class QUdpSocket;


namespace SST {

class Stream;
class StreamServer;
class TcpStream;
class TcpServer;
class Host;


// An instance of this class serves one client socket.
class SockServer : public QObject
{
	Q_OBJECT

private:
	QIODevice *dev;

public:
	SockServer(QObject *parent, QIODevice *dev);

private slots:
	void gotReadyRead();
	void gotDisconnected();
	void gotSubstream();
};


class TestServer : public QObject
{
	Q_OBJECT

	Host *host;

	StreamServer *sstsrv;		// Userland SST server
	TcpServer *tcpsrv;		// Userland TCP server
	QTcpServer *natsrv;		// Native TCP server socket
	QUdpSocket *udpsock;		// Native UDP socket

public:
	TestServer(Host *host, quint16 port);

private slots:
	void sstConnection();
	void tcpConnection(TcpStream *strm);
	void natPending();
	void udpReadyRead();
};

} // namespace SST

#endif	// SRV_H
