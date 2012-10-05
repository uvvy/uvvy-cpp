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

#include <netinet/in.h>

#include <QTcpSocket>
#include <QTcpServer>
#include <QtDebug>

#include "stream.h"
#include "util.h"

#include "main.h"
#include "srv.h"
#include "tcp.h"

using namespace SST;


// True to fill responses with pseudorandom (uncompressible) data,
// false just to send anything that happens to be in memory...
bool randfill = false;


////////// SockServer //////////

SockServer::SockServer(QObject *parent, QIODevice *dev)
:	QObject(parent), dev(dev)
{
}

void SockServer::gotReadyRead()
{
	while (true) {
		// Get one REQLEN-byte request at a time
		QByteArray req;
		req.resize(REQLEN);
		int act = dev->read(req.data(), REQLEN);
		if (act == 0)
			return;
		Q_ASSERT(act == REQLEN); // not correct, but OK for tests

		// The request is the number of reply bytes the client wants.
		int replen = ntohl(*(quint32*)req.data());
		//qDebug() << "got request for" << replen << "bytes"
		//	<< "on socket" << this;

		if (testproto == TESTPROTO_SST) {
			int pri = ntohl(((quint32*)req.data())[1]);
			((Stream*)dev)->setPriority(pri);
		}

		// Form a response buffer of random data.
		QByteArray repl;
		if (randfill)
			repl = pseudoRandBytes(replen);
		else
			repl.resize(replen);

		// Ship it
		act = dev->write(repl);
		Q_ASSERT(act == replen);

#if 0
		// Just for kicks, try closing the socket abruptly now...
		while (dev->bytesToWrite() > 0)
			dev->waitForBytesWritten(-1);
		qDebug() << "buffered:"
				<< !(dev->openMode() & QIODevice::Unbuffered)
			<< "toWrite:" << dev->bytesToWrite();
		((QAbstractSocket*)dev)->abort();
		deleteLater();
		return;
#endif
	}
}

void SockServer::gotDisconnected()
{
	qDebug("SockServer disconnected");
	deleteLater();
}

void SockServer::gotSubstream()
{
	Stream *strm = (Stream*)dev;
	Stream *substrm = strm->acceptSubstream();
	if (!substrm)
		return;

	qDebug() << "Got priority change request";
	qint32 buf;
	int act = substrm->read((char*)&buf, 4);
	Q_ASSERT(act == 4);

	int newpri = ntohl(buf);
	strm->setPriority(newpri);

	// We're done with the substream now...
	delete substrm;

	// Check for more incoming substreams
	return gotSubstream();
}


////////// TestServer //////////

TestServer::TestServer(Host *host, quint16 port)
:	host(host)
{
	switch (testproto) {
	case TESTPROTO_SST:
		sstsrv = new StreamServer(host, this);
		if (!sstsrv->listen("ucbtest", "UCB web client trace test",
				"ucbtestproto", "UCB trace test protocol"))
			qFatal("Can't listen on 'ucbtest' service name");
		connect(sstsrv, SIGNAL(newConnection()),
			this, SLOT(sstConnection()));
		break;
	case TESTPROTO_TCP:
		tcpsrv = new TcpServer(host, this);
		connect(tcpsrv, SIGNAL(newConnection(TcpStream *)),
			this, SLOT(tcpConnection(TcpStream *)));
		tcpsrv->listen();
		break;
	case TESTPROTO_NAT:
		natsrv = new QTcpServer(this);
		if (!natsrv->listen(QHostAddress::Any, port))
			qFatal("Can't listen on port %d", port);
		connect(natsrv, SIGNAL(newConnection()),
			this, SLOT(natPending()));
		break;
	case TESTPROTO_UDP:
		udpsock = new QUdpSocket(this);
		if (!udpsock->bind(QHostAddress::Any, port+2))
			qFatal("Can't bind to port %d: %s", port+2,
				udpsock->errorString().toLocal8Bit().data());
		connect(udpsock, SIGNAL(readyRead()),
			this, SLOT(udpReadyRead()));
		break;
	}
}

void TestServer::sstConnection()
{
	Stream *strm = sstsrv->accept();
	if (!strm)
		return;

	SockServer *ss = new SockServer(this, strm);
	connect(strm, SIGNAL(readyRead()),
		ss, SLOT(gotReadyRead()));
	connect(strm, SIGNAL(disconnected()),
		ss, SLOT(gotDisconnected()));
	connect(strm, SIGNAL(newSubstream(Stream*)),
		ss, SLOT(gotSubstream(Stream*)), Qt::QueuedConnection);
	strm->setParent(ss);
	strm->listen(Stream::Unlimited);

	// Check for more queued incoming connections
	sstConnection();
}

void TestServer::tcpConnection(TcpStream *strm)
{
	SockServer *ss = new SockServer(this, strm);
	connect(strm, SIGNAL(readyRead()),
		ss, SLOT(gotReadyRead()));
	connect(strm, SIGNAL(disconnected()),
		ss, SLOT(gotDisconnected()));
	strm->setParent(ss);
}

void TestServer::natPending()
{
	while (true) {
		QTcpSocket *sock = natsrv->nextPendingConnection();
		if (!sock)
			return;

		SockServer *ss = new SockServer(this, sock);
		connect(sock, SIGNAL(readyRead()),
			ss, SLOT(gotReadyRead()));
		connect(sock, SIGNAL(disconnected()),
			ss, SLOT(gotDisconnected()));
		sock->setParent(ss);
	}
}

void TestServer::udpReadyRead()
{
	while (true) {
		QByteArray req;
		req.resize(REQLEN);
		Endpoint ep;
		qint64 act = udpsock->readDatagram(req.data(), REQLEN,
						&ep.addr, &ep.port);
		if (act < 0)
			return;
		if (act != REQLEN)
			return qDebug("request with bad length");

		int replen = ntohl(*(quint32*)req.data());
		qDebug("Sending %d-byte UDP datagram", replen);

		QByteArray repl(pseudoRandBytes(replen));
		if (udpsock->writeDatagram(repl, ep.addr, ep.port) < 0)
			qDebug("Send failed locally!");
	}
}

