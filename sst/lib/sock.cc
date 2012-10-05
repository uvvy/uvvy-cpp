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


#include <QDataStream>
#include <QSettings>
#include <QtDebug>

#include "util.h"
#include "sock.h"
#include "xdr.h"
#include "os.h"

using namespace SST;


////////// SocketEndpoint //////////

SocketEndpoint::SocketEndpoint(const Endpoint &ep, Socket *s)
:	Endpoint(ep),
	sock(s)
{
	Q_ASSERT(sock != NULL);
}

bool SocketEndpoint::send(const char *data, int size) const
{
	if (!sock) {
		qDebug("Trying to send on a nonexistent socket!");
		return false;
	}
	return sock->send(*this, data, size);
}

QString SocketEndpoint::toString() const
{
	return Endpoint::toString() + " via "
		+ (sock.isNull() ? "NULL" : sock->toString());
}

uint qHash(const SST::SocketEndpoint &sep)
{
	const Endpoint &ep = sep;
	Socket *sock = sep.sock;
	return qHash(ep) + qHash(sock);
}


////////// Socket //////////

Socket::~Socket()
{
	//qDebug() << this << "~Socket()";

	// Unbind all flows
	foreach (SocketFlow *f, flows.values())
		f->unbind();
	Q_ASSERT(flows.isEmpty());
}

void Socket::setActive(bool newact)
{
	if (newact && !act) {
		h->actsocks.append(this);
	} else if (act && !newact) {
		h->actsocks.removeAll(this);
	}
	act = newact;

	h->activeSocketsChanged();
}

bool
Socket::bindFlow(const Endpoint &remoteep, Channel localchan, SocketFlow *fl)
{
	Q_ASSERT(flow(remoteep, localchan) == NULL);

	QPair<Endpoint,Channel> p(remoteep, localchan);
	flows.insert(p, fl);
	return true;
}

void
Socket::receive(QByteArray &msg, const SocketEndpoint &src)
{
	if (msg.size() < 4) {
		// Message too small to be interesting
		qDebug("Ignoring runt UDP datagram");
		return;
	}

	// First interpret the first byte as a channel number
	// to try to find an endpoint-specific flow.
	Channel chan = msg.at(0);
	SocketFlow *fl = flow(src, chan);
	if (fl != NULL)
		return fl->receive(msg, src);

	// If that doesn't work, it may be a global control packet:
	// if so, pass it to the appropriate SocketReceiver.
	XdrStream rs(&msg, QIODevice::ReadOnly);
	quint32 magic;
	rs >> magic;
	SocketReceiver *rcv = h->receivers.value(magic);
	if (rcv)
		return rcv->receive(msg, rs, src);

	qDebug("Received control message for unknown flow/receiver %08x",
		magic);
}

bool Socket::isCongestionControlled(const Endpoint &)
{
	return false;
}

int Socket::mayTransmit(const Endpoint &)
{
	qFatal("Socket::mayTransmit() called on non-flow-controlled socket");
	return -1;
}

QString Socket::toString() const
{
	return QString("%1(0x%2)")
		.arg(metaObject()->className())
		.arg((ulong)this, 0, 16);
}


////////// UdpSocket //////////

UdpSocket::UdpSocket(SocketHostState *host, QObject *parent)
:	Socket(host, parent)
{
	connect(&usock, SIGNAL(readyRead()), this, SLOT(udpReadyRead()));
}

bool UdpSocket::bind(const QHostAddress &addr, quint16 port,
			QUdpSocket::BindMode mode)
{
	Q_ASSERT(!active());

	if (!usock.bind(addr, port, mode))
		return false;

	setActive(true);
	return true;
}

bool
UdpSocket::send(const Endpoint &ep, const char *data, int size)
{
	// XXX Qt bug?  For some reason, at least under both Linux & Mac,
	// our UDP socket is getting mysteriously "unbound"
	// when we try to send a packet to an IPv6 address...
	// localPort() still reports the correct port,
	// but after our first attempt to send an IPv6 packet,
	// all the subsequent IPv4 packets we send
	// appear on the wire with newly-allocated port numbers.
	if (ep.addr.protocol() != QAbstractSocket::IPv4Protocol)
		return false;

	bool rc = usock.writeDatagram(data, size, ep.addr, ep.port) == size;
	if (!rc)
		qDebug() << "Socket::send:" << errorString();
//	qDebug() << "after writeDatagram: rc" << rc
//		<< "err" << errorString() << "state" << state()
//		<< "valid" << isValid() << "addr" << localAddress().toString()
//		<< "port" << localPort();
	return rc;
}

void
UdpSocket::udpReadyRead()
{
	SocketEndpoint src;
	src.sock = this;
	QByteArray msg;
	int size;
	while ((size = usock.pendingDatagramSize()) >= 0) {

		// Read the datagram
		msg.resize(size);
		if (usock.readDatagram(msg.data(), size, &src.addr, &src.port)
				!= size) {
			qWarning("Error reading %d-byte UDP datagram", size);
			break;
		}

		receive(msg, src);
	}
}

QList<Endpoint> UdpSocket::localEndpoints()
{
	QList<QHostAddress> addrs = localHostAddrs();
	quint16 port = usock.localPort();
	Q_ASSERT(port > 0);

	QList<Endpoint> eps;
	foreach (const QHostAddress &addr, addrs) {
		qDebug() << "Local endpoint"
			<< Endpoint(addr, port).toString();
		eps.append(Endpoint(addr, port));
	}
	return eps;
}


////////// SocketFlow //////////

SocketFlow::SocketFlow(QObject *parent)
:	QObject(parent),
	sock(NULL),
	localchan(localchan),
	remotechan(0),
	active(false)
{
}

SocketFlow::~SocketFlow()
{
	// Stop and unbind this flow
	unbind();
}

Channel SocketFlow::bind(Socket *sock, const Endpoint &dst)
{
	Q_ASSERT(sock);
	Q_ASSERT(!active);	// can't bind while flow is active
	Q_ASSERT(!this->sock);	// can't bind again while already bound

	// Find a free channel number for this remote endpoint.
	// Never assign channel zero - that's reserved for control packets.
	Channel chan = 1;
	while (sock->flow(dst, chan) != NULL) {
		if (++chan == 0)
			return 0;	// wraparound - no channels available
	}

	// Bind to this channel
	if (!bind(sock, dst, chan))
		return 0;

	return chan;
}

bool SocketFlow::bind(Socket *sock, const Endpoint &dst, Channel chan)
{
	Q_ASSERT(sock);
	Q_ASSERT(!active);	// can't bind while flow is active
	Q_ASSERT(!this->sock);	// can't bind again while already bound

	if (sock->flow(dst, chan) != NULL)
		return false;		// Already in use

	//qDebug() << this << "bind" << dst << "chan" << chan << "to" << sock;

	// Bind us to this socket and channel
	remoteep.addr = dst.addr;
	remoteep.port = dst.port;
	localchan = chan;
	if (!sock->bindFlow(remoteep, localchan, this))
		return false;

	this->sock = sock;
	return true;
}

void SocketFlow::start(bool)
{
	Q_ASSERT(remotechan);

	active = true;
}

void SocketFlow::stop()
{
	active = false;
}

void SocketFlow::unbind()
{
	stop();
	Q_ASSERT(!active);

	if (sock) {
		//qDebug() << this << "unbind from sock" << sock;
		QPair<Endpoint,Channel> p(remoteep, localchan);
		Q_ASSERT(sock->flows.value(p) == this);
		sock->flows.remove(p);

		sock = NULL;
		localchan = 0;
	}
}

void SocketFlow::receive(QByteArray &msg, const SocketEndpoint &src)
{
	received(msg, src);
}

int SocketFlow::mayTransmit()
{
	Q_ASSERT(sock);
	return sock->mayTransmit(remoteep);
}


////////// SocketReceiver //////////

SocketReceiver::~SocketReceiver()
{
	unbind();
}

void SocketReceiver::bind(quint32 magic)
{
	Q_ASSERT(!isBound());

	// Receiver's magic value must leave the upper byte 0
	// to distinguish control packets from flow data packets.
	Q_ASSERT(magic <= 0xffffff);

	// Make sure we don't try to enter two conflicting receivers
	Q_ASSERT(!h->receivers.contains(magic));

	h->receivers.insert(mag = magic, this);
}

void SocketReceiver::unbind()
{
	if (isBound())  {
		Q_ASSERT(mag);
		Q_ASSERT(h->receivers.value(mag) == this);
		h->receivers.remove(mag);
		mag = 0;
	}
}


////////// SocketHostState //////////

SocketHostState::~SocketHostState()
{
}

QList<Endpoint> SocketHostState::activeLocalEndpoints()
{
	QList<Endpoint> l;
	foreach (Socket *sock, activeSockets()) {
		Q_ASSERT(sock->active());
		l += sock->localEndpoints();
	}
	return l;
}

Socket *SocketHostState::newSocket(QObject *parent)
{
	return new UdpSocket(this, parent);
}

Socket *
SocketHostState::initSocket(QSettings *settings, int defaultport)
{
	if (mainsock && mainsock->active())
		return mainsock;	// Already initialized

	// See if a port number is recorded in our settings;
	// if so, use that instead of the specified default port.
	if (settings) {
		int port = settings->value("port").toInt();
		if (port > 0 && port <= 65535)
			defaultport = port;
	}

	// Create and bind the main socket.
	mainsock = newSocket(this);
	if (!mainsock->bind(QHostAddress::Any, defaultport,
				QUdpSocket::DontShareAddress)) {
		qWarning("Can't bind to port %d - trying another",
			defaultport);
		if (!mainsock->bind(QHostAddress::Any, 0,
					QUdpSocket::DontShareAddress))
			qFatal("Can't bind main socket: %s",
				mainsock->errorString()
					.toLocal8Bit().data());
		defaultport = mainsock->localPort();
	}

	// Remember the port number we ended up using.
	if (settings)
		settings->setValue("port", defaultport);
	qDebug("Bound to port %d", defaultport);

	return mainsock;
}

