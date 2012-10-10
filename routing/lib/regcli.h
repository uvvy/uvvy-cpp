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
#ifndef SST_REGCLI_H
#define SST_REGCLI_H

#include <QSet>

#include "reg.h"
#include "sock.h"
#include "timer.h"

class QHostInfo;


namespace SST {

class Host;
class RegReceiver;

class RegClient : public QObject
{
	friend class RegReceiver;
	Q_OBJECT

private:
	// Max time before rereg - 1 hr
	static const qint64 maxRereg = (qint64)60*60*1000000;


	Host *const h;		// Pointer to our per-host state

	enum State {
		Idle = 0,	// Unregistered and not doing anything
		Resolve,	// Resolving rendezvous server's host name
		Insert1,	// Sent Insert1 request, waiting response
		Insert2,	// Sent Insert2 request, waiting response
		Registered,	// Successfully registered
	} state;

	// DNS resolution info
	QString srvname;	// DNS hostname or IP address of server
	quint16 srvport;	// Port number of registration server
	int lookupid;		// QHostInfo lookupId for DNS resolution
	QList<QHostAddress> addrs;	// Server addresses from resolution
	RegInfo inf;		// Registration metadata

	// Registration process state
	QByteArray idi;		// Initiator's identity (i.e., mine)
	QByteArray ni;		// Initiator's nonce
	QByteArray nhi;		// Initiator's hashed nonce
	QByteArray chal;	// Responder's challenge from Insert1 reply
	QByteArray key;		// Our encoded public key to send to server
	QByteArray sig;		// Our signature to send in Insert2

	// Outstanding lookups and searches for which we're awaiting replies.
	QSet<QByteArray> lookups;	// IDs we're doing lookups on
	QSet<QByteArray> punches;	// Lookups with notify requests
	QSet<QString> searches;		// Strings we're searching for

	// Retry state
	Timer retrytimer;	// Retransmission timer
	bool persist;		// True if we should never give up

	Timer reregtimer;	// Counts lifetime of our reg entry

	// Error state
	QString err;


	// As the result of an error, disconnect and notify the client.
	void fail(const QString &error);

public:
	RegClient(Host *h, QObject *parent = NULL);
	~RegClient();

	// Set the metadata to attach to our registration
	inline RegInfo info() { return inf; }
	inline void setInfo(const RegInfo &info) { inf = info; }

	// Attempt to register with the specified registration server.
	// We'll send a stateChanged() signal when it succeeds or fails.
	void registerAt(const QString &srvhost,
			quint16 port = REGSERVER_DEFAULT_PORT);

	// Attempt to re-register with the same server previously indicated.
	void reregister();

	inline QString serverName() { return srvname; }
	inline quint16 serverPort() { return srvport; }

	inline QString errorString() { return err; }
	inline void setErrorString(const QString &err) { this->err = err; }

	inline bool idle() { return state == Idle; }
	inline bool registered() { return state == Registered; }
	inline bool registering() { return !idle() && !registered(); }

	// A persistent RegClient will never give up trying to register,
	// and will try to re-register if its connection is lost.
	inline void setPersistent(bool persist) { this->persist = persist; }
	inline bool isPersistent() { return persist; }

	// Request information about a specific ID from the server.
	// Will send a lookupDone() signal when the request completes.
	// If 'notify', ask regserver to notify the target as well.
	// Must be in the registered() state to initiate a lookup.
	void lookup(const QByteArray &id, bool notify = false);

	// Search for IDs of clients with metadata matching a search string.
	// Will send a searchDone() signal when the request completes.
	// Must be in the registered() state to initiate a search.
	void search(const QString &text);

	// Disconnect from our server or cancel the registration process,
	// and return immediately to the idle state.
	void disconnect();


signals:
	void stateChanged();
	void lookupDone(const QByteArray &id, const Endpoint &loc,
			const RegInfo &info);
	void lookupNotify(const QByteArray &id, const Endpoint &loc,
			const RegInfo &info);
	void searchDone(const QString &text, const QList<QByteArray> ids,
			bool complete);

private:
	void goInsert1();
	void sendInsert1();
	void gotInsert1Reply(XdrStream &rs);

	void goInsert2();
	void sendInsert2();
	void gotInsert2Reply(XdrStream &rs);

	void sendLookup(const QByteArray &id, bool notify);
	void gotLookupReply(XdrStream &rs, bool isnotify);

	void sendSearch(const QString &text);
	void gotSearchReply(XdrStream &rs);

	void send(const QByteArray &msg);


private slots:
	void resolveDone(const QHostInfo &hi);	// DNS lookup done
	void timeout(bool fail);		// Retry timer timeout
	void reregTimeout();			// Reregister timeout
};

// Private helper class for RegClient -
// attaches to our Socket and dispatches control messages to different clients
class RegReceiver : public SocketReceiver
{
	friend class RegClient;
	friend class RegHostState;

private:
	// Global hash table of active RegClient instances,
	// for dispatching incoming messages based on hashed nonce (nhi).
	QHash<QByteArray,RegClient*> nhihash;


	RegReceiver(Host *h);
	virtual void receive(QByteArray &msg, XdrStream &ds,
				const SocketEndpoint &src);
};

// Per-host state for the registration client.
class RegHostState : public QObject
{
	friend class RegClient;
	Q_OBJECT

private:
	RegReceiver rcvr;

	// Global registry of every RegClient for this host, so we can
	// produce signals when RegClients are created or destroyed.
	QSet<RegClient*> cliset;

public:
	inline RegHostState(Host *h) : rcvr(h) { }

	inline QList<RegClient*> regClients()
		{ return cliset.toList(); }

signals:
	void regClientCreate(RegClient *rc);
	void regClientDestroy(RegClient *rc);
};

} // namespace SST

#endif	// SST_REGCLI_H
