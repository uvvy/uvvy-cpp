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
//
// Class implementing a UDP socket for Netsteria use.
// Multiplexes between flow-setup/key exchange traffic (which goes to key.cc)
// and per-flow data traffic (which goes to flow.cc).
// XX Rename Socket* to Net* or Link*?
// 
#ifndef SST_SOCK_H
#define SST_SOCK_H

#include <QHash>
#include <QPair>
#include <QPointer>
#include <QHostAddress>
#include <QUdpSocket>
#include <QPointer>

#include "util.h"

#define NETSTERIA_DEFAULT_PORT	8661

class QSettings;


namespace SST {

class XdrStream;
class Socket;
class SocketFlow;
class SocketReceiver;
class SocketHostState;


// SST expresses current link status as one of three states:
//	- LinkUp: apparently alive, all's well as far as we know.
//	- LinkStalled: briefly lost connectivity, but may be temporary.
//	- LinkDown: definitely appears to be down for the count.
enum LinkStatus {
	LinkDown,
	LinkStalled,
	LinkUp
};


// An 8-bit channel number distinguishes different flows
// between the same pair of socket-layer endpoints.
// Channel number 0 is always invalid.
typedef quint8 Channel;


// SockEndpoint builds on the basic Endpoint class
// to keep an association with a particular Socket as well.
struct SocketEndpoint : public Endpoint
{
	QPointer<Socket> sock;

	inline SocketEndpoint() { }
	inline SocketEndpoint(const SocketEndpoint &other)
		: Endpoint(other), sock(other.sock) { }
	SocketEndpoint(const Endpoint &ep, Socket *sock);

	// Send a message to this endpoint on this socket
	bool send(const char *data, int size) const;
	inline bool send(const QByteArray &msg) const
		{ return send(msg.constData(), msg.size()); }

	inline bool operator==(const SocketEndpoint &other) const
		{ const Endpoint &ep = *this, &oep = other;
		  return ep == oep && &*sock == &*other.sock; }
	inline bool operator!=(const SocketEndpoint &other) const
		{ return !(*this == other); }

	QString toString() const;
};


/** Abstract base class representing network attachments
 * for the SST protocols to use.
 * @see UdpSocket
 */
class Socket : public QObject
{
	friend class SocketFlow;
	Q_OBJECT

private:
	/// Host state instance this Socket is attached to
	SocketHostState *const h;

	/// Lookup table of flows currently attached to this socket.
	QHash<QPair<Endpoint,Channel>, SocketFlow*> flows;

	/// True if this socket is fair game for use by upper level protocols.
	bool act;


public:
	inline Socket(SocketHostState *host, QObject *parent = NULL)
		: QObject(parent), h(host), act(false) { }
	virtual ~Socket();

	/** Determine whether this socket is active.
	 * Only active sockets are returned by SocketHostState::activeSockets().
	 * @return true if socket is active. */
	inline bool active() { return act; }

	/** Activate or deactivate this Socket.
	 * Only active sockets are returned from calls to activeSockets().
	 * @param act true if the socket should be marked active. */
	void setActive(bool act);

	/** Bind this socket to a local port and activate it if successful.
	 * @param addr the address to bind to, normally QHostAddress::Any.
	 * @param port the port to bind to, 0 for any port.
	 * @param mode how to bind the socket - see QUdpSocket::BindMode.
	 */
	virtual bool bind(const QHostAddress &addr = QHostAddress::Any,
		quint16 port = 0,
		QUdpSocket::BindMode mode = QUdpSocket::DefaultForPlatform) = 0;

	/** Send a packet on this socket.
	 * @param ep the destination address to send the packet to
	 * @param msg the packet data
	 * @return true if send was successful */
	virtual bool send(const Endpoint &ep, const char *data, int size) = 0;
	inline bool send(const Endpoint &ep, const QByteArray &msg)
		{ return send(ep, msg.constData(), msg.size()); }

	/** Find all known local endpoints referring to this socket.
	 * @return a list of Endpoint objects. */
	virtual QList<Endpoint> localEndpoints() = 0;

	virtual quint16 localPort() = 0;

	/// Find flow associations attached to this socket.
	inline SocketFlow *flow(const Endpoint &dst, Channel chan)
		{ return flows.value(QPair<Endpoint,Channel>(dst, chan)); }

	/// Return a description of any error detected on bind() or send().
	virtual QString errorString() = 0;


	// Returns true if this socket provides flow/congestion control
	// when communicating with the specified remote endpoint
	virtual bool isCongestionControlled(const Endpoint &ep);

	// For flow/congestion-controlled sockets,
	// returns the number of packets that may be transmitted now
	// to a particular target endpoint
	virtual int mayTransmit(const Endpoint &ep);

	virtual QString toString() const;


protected:

	/** Implementation subclass calls this method with received packets.
	 * @param msg the packet received
	 * @param src the source from which the packet arrived */
	void receive(QByteArray &msg, const SocketEndpoint &src);

	/** Bind a new SocketFlow to this Socket.
	 * Called by SocketFlow::bind() to register in the table of flows.
	 */
	virtual bool bindFlow(const Endpoint &remoteep, Channel localchan,
				SocketFlow *flow);
};


/// Main class representing a UDP socket running our transport protocol.
class UdpSocket : public Socket
{
	Q_OBJECT

	QUdpSocket usock;

public:
	UdpSocket(SocketHostState *host, QObject *parent = NULL);

	/** Bind this UDP socket to a port and activate it if successful.
	 * @param addr the address to bind to, normally QHostAddress::Any.
	 * @param port the port to bind to, 0 for any port.
	 * @param mode how to bind the socket - see QUdpSocket::BindMode.
	 */
	bool bind(const QHostAddress &addr = QHostAddress::Any,
		quint16 port = 0,
		QUdpSocket::BindMode mode = QUdpSocket::DefaultForPlatform);

	// Send a packet on this UDP socket.
	// Implements of Socket::send().
	bool send(const Endpoint &ep, const char *data, int size);

	// Return all known local endpoints referring to this socket.
	// Implements Socket::localEndpoints().
	QList<Endpoint> localEndpoints();

	quint16 localPort() { return usock.localPort(); }

	/// Return a description of any error detected on bind() or send().
	inline QString errorString() { return usock.errorString(); }


private slots:
	void udpReadyRead();
};


// Base class for flows that may be bound to a socket,
// for dispatching received packets based on endpoint and channel number.
// May be used as an abstract base by overriding the receive() method,
// or used as a concrete class by connecting to the received() signal.
class SocketFlow : public QObject
{
	friend class Socket;
	Q_OBJECT

private:
	Socket *sock;		// Socket we're currently bound to, if any
	Endpoint remoteep;	// UDP endpoint of remote end
	Channel localchan;	// Channel number of this flow at local node
	Channel remotechan;	// Channel number of this flow at remote node
	bool active;		// True if we're sending and accepting packets

public:
	SocketFlow(QObject *parent = NULL);
	virtual ~SocketFlow();

	// Set up for communication with specified remote endpoint,
	// allocating and binding a local channel number in the process.
	// Returns 0 if no channels are available for specified endpoint.
	Channel bind(Socket *sock, const Endpoint &remoteep);
	inline Channel bind(const SocketEndpoint &remoteep)
		{ return bind(remoteep.sock, remoteep); }

	// Bind to a particular channel number.
	// Returns false if the channel is already in use.
	bool bind(Socket *sock, const Endpoint &remoteep, Channel chan);

	// Set the channel number to direct packets to the remote endpoint.
	// This MUST be done before a new flow can be activated.
	inline void setRemoteChannel(Channel chan) { remotechan = chan; }

	// Return the remote endpoint we're bound to, if any
	inline SocketEndpoint remoteEndpoint()
		{ return SocketEndpoint(remoteep, sock); }

	// Return current local and remote channel numbers
	inline Channel localChannel() { return localchan; }
	inline Channel remoteChannel() { return remotechan; }

	// Start or stop the flow.
	virtual void start(bool initiator);
	virtual void stop();

	// Return infrormation about flow state.
	inline bool isActive() { return active; }
	inline bool isBound() { return sock != NULL; }

	// Test whether underlying socket is already congestion controlled
	inline bool isSocketCongestionControlled()
		{ return sock->isCongestionControlled(remoteep); }

	// Stop flow and unbind from any currently bound remote endpoint.
	void unbind();

protected:

	inline bool udpSend(QByteArray &pkt) const
		{ Q_ASSERT(active); return sock->send(remoteep, pkt); }

	virtual void receive(QByteArray &msg, const SocketEndpoint &src);

	// When the underlying socket is already flow/congestion-controlled,
	// this function returns the number of packets
	// that flow control says we may transmit now, 0 if none.
	virtual int mayTransmit();

signals:
	void received(QByteArray &msg, const SocketEndpoint &src);

	// Signalled when flow/congestion control may allow new transmission
	void readyTransmit();
};


// SocketReceiver is an abstract base class for control protocols
// that need to multiplex onto Netsteria sockets:
// e.g., the key exchange protocol (key*) and registration protocol (reg*).
// A control protocol is identified by a 32-bit magic value,
// whose topmost byte must be zero to distinguish it from flows.
class SocketReceiver : public QObject
{
	friend class Socket;

	SocketHostState *const h;
	quint32 mag;

protected:
	void bind(quint32 magic);
	void unbind();

	inline bool isBound() { return mag != 0; }
	inline quint32 magic() { return mag; }

	// Socket calls this method to dispatch control messages.
	// The supplied XdrStream is positioned just after the
	// 4-byte magic value identifying the control protocol.
	virtual void receive(QByteArray &msg, XdrStream &ds,
				const SocketEndpoint &src) = 0;

	inline SocketReceiver(SocketHostState *h, QObject *parent = NULL)
		: QObject(parent), h(h), mag(0) { }
	inline SocketReceiver(SocketHostState *h, quint32 magic,
				QObject *parent = NULL)
		: QObject(parent), h(h), mag(0) { bind(magic); }
	virtual ~SocketReceiver();
};


/** Per-host state for the Socket module.
 * This class encapsulates all the state for this module
 * that would normally be held in global/static variables.
 * @see Host */
class SocketHostState : public QObject
{
	friend class Socket;
	friend class SocketReceiver;

	Q_OBJECT

	/** List of all currently-active sockets. */
	QList<Socket*> actsocks;

	/** Socket created by init(), if any. */
	QPointer<Socket> mainsock;

	/** Lookup table of all registered SocketReceivers for this host,
	 * keyed on their 24-bit magic control packet type. */
	QHash<quint32, SocketReceiver*> receivers;


public:
	inline SocketHostState() : mainsock(NULL) { }
	virtual ~SocketHostState();

	/** Obtain a list of all currently active sockets.
	 * Used by upper-level protocols (e.g., key exchange, registration)
	 * to send out initial discovery messages on all available sockets.
	 * Subsequent messages normally get sent only to
	 * the specific socket a discovery response was seen on.
	 * @return a list of pointers to each currently active Socket. */
	inline QList<Socket*> activeSockets()
		{ return actsocks; }

	/// Get a list of all known local endpoints for all active sockets.
	QList<Endpoint> activeLocalEndpoints();

	inline SocketReceiver *lookupReceiver(quint32 magic)
		{ return receivers.value(magic); }


	/** Create a new network Socket.
	 * The default implementation creates a UdpSocket,
	 * but this may be overridden to virtualize the network. */
	virtual Socket *newSocket(QObject *parent = NULL);

	/** Create and get at least one Socket up and running.
	 * This function only creates one socket
	 * no matter how many times it is called.
	 * It exits the application via qFatal() if socket creation fails.
	 * @param host the Host instance the socket should be attached to.
	 * @param settings if non-NULL, init() looks for a 'port' tag
	 *	and uses it in place of the specified default port if found.
	 *	In any case, sets the 'port' tag to the port actually used.
	 * @param defaultport the default port to bind to
	 *	if no 'port' tag is found in the @a settings.
	 * @return the Socket created (during this or a previous call).
	 */
	Socket *initSocket(QSettings *settings = NULL,
		int defaultport = NETSTERIA_DEFAULT_PORT);

signals:
	/** This signal is sent whenever the host's set of
	 * active sockets changes. */
	void activeSocketsChanged();
};

} // namespace SST


// Hash function for SocketEndpoint structs
uint qHash(const SST::SocketEndpoint &ep);

// Hash function for (Endpoint,Channel) tuples
inline uint qHash(const QPair<SST::Endpoint,SST::Channel> fl)
	{ return qHash(fl.first) + qHash(fl.second); }


#endif	// SST_SOCK_H
