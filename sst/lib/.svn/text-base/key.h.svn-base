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
#ifndef SST_KEY_H
#define SST_KEY_H

#include <QHash>
#include <QMultiHash>
#include <QPointer>

#include <openssl/dh.h>

#include "sock.h"
#include "timer.h"


namespace SST {

// Control chunk magic value for the Netsteria key exchange protocol.
// The upper byte is zero to distinguish control packets from flow packets.
// 0x4e6b78 = 'Nkx': 'Netsteria key exchange'
//#define KEY_MAGIC	(qint32)0x004e6b78


// Bit mask of allowable key security methods
#define KEYMETH_NONE		0x0001	// No security at all
#define KEYMETH_CHK		0x0002	// Weak 32-bit keyed checksum
#define KEYMETH_SHA256		0x0010	// HMAC-SHA256 auth, DH key agreement
#define KEYMETH_AES		0x0020	// AES enc, HMAC-SHA256 auth, DH
#define KEYMETH_DEFAULT		KEYMETH_AES	// Secure by default


// Well-known control chunk types for keying
#define KEYCHUNK_NI		0x0001	// Multi-cyphersuite initiator nonce
#define KEYCHUNK_JFDH_R0	0x0010	// DH-based JFK key agreement
#define KEYCHUNK_JFDH_I1	0x0011
#define KEYCHUNK_JFDH_R1	0x0012
#define KEYCHUNK_JFDH_I2	0x0013
#define KEYCHUNK_JFDH_R2	0x0014


class Host;
class Flow;
class KeyInitiator;
class KeyResponder;
class KeyHostState;
class DHKey;
class ChecksumArmor;

class KeyChunkChkI1Data;
class KeyChunkChkR1Data;
class KeyChunkDhI1Data;
class KeyChunkDhR1Data;
class KeyChunkDhI2Data;
class KeyChunkDhR2Data;


// This class manages the initiator side of the key exchange.
// We create one instance for each outgoing connection we attempt.
//
// XXX we should really have a separate Idle state,
// so that clients can hookup signals before starting key exchange.
// XXX make KeyInitiator an abstract base class like KeyResponder,
// calling a newFlow() method when it needs to set up a flow
// rather than requiring the flow to be passed in at the outset.
class KeyInitiator : public QObject
{
	Q_OBJECT
	friend class KeyResponder;

	enum State {
		I1,
		I2,
		Done
	};

	// Basic parameters
	Host *const h;		// Our per-host state
	Flow *const fl;		// Flow we're trying to set up
	SocketEndpoint sepr;	// Remote endpoint we're trying to contact
	QByteArray idr;		// Target's host ID (empty if unspecified)
	QByteArray ulpi;	// Opaque info block to pass to responder
	const quint32 magic;	// Magic identifier for upper-layer protocol
	quint32 methods;	// Security methods allowed


	////////// Weak keyed checksum negotiation //////////

	quint32 chkkey;		// Checksum key
	QByteArray cookie;	// Responder's cookie


	////////// AES/SHA256 with DH key agreement //////////

	quint8 dhgroup;		// DH group to use
	quint8 keylen;		// AES key length to use

	// Protocol state set up before sending I1
	State state;
	bool early;		// Still early enough to cancel asynchronously
	QByteArray ni, nhi;	// Initiator's nonce, and hashed nonce
	QByteArray dhi;		// Initiator's DH public key

	// Set on receipt of R1 response
	QByteArray nr;		// Responder's nonce
	QByteArray dhr;		// Responder's DH public key
	QByteArray hhkr;	// Responder's challenge cookie
	QByteArray master;	// Shared master secret
	QByteArray encidi;	// Encrypted and authenticated identity info

	// Retransmit state
	Timer txtimer;


public:
	// Start key negotiation for a Flow
	// that has been bound to a socket but not yet activated.
	// If 'idr' is non-empty, only connect to specified host ID.
	// The KeyInitiator makes itself the parent of the provided Flow,
	// so that if it is deleted the incomplete Flow will be too.
	// The client must therefore re-parent the flow
	// after successful key exchange before deleting the KeyInitiator.
	KeyInitiator(Flow *flow, quint32 magic,
			const QByteArray &idr = QByteArray(),
			quint8 dhgroup = 0);
	~KeyInitiator();

	inline Host *host() { return h; }

	inline Flow *flow() { return fl; }
	inline bool isDone() { return state == Done; }

	// Returns true if this KeyInitiator hasn't gotten far enough
	// so that the remote peer might possibly create permanent state
	// if we cancel the process at our end now.
	// We use this if we're trying to initiate a connection to a peer
	// but that peer contacts us first, giving us a primary flow;
	// we can then abort our outstanding active initiation attempts
	// ONLY if they're still in an early enough stage that we know
	// the responder won't be left with a dangling end of a new flow.
	inline bool isEarly() { return early; }

	inline SocketEndpoint remoteEndpoint() { return sepr; }

	// Set/get the opaque information block to pass to the responder.
	// the info block is passed encrypted and authenticated in our I2;
	// the responder can use it to decide whether to accept the connection
	// and to setup any upper-layer protocol parameters for the new flow.
	inline QByteArray info() { return ulpi; }
	inline void setInfo(const QByteArray &info) { ulpi = info; }

	// Cancel all of this KeyInitiator's activities
	// (without actually deleting the object just yet).
	void cancel();

signals:
	void completed(bool success);

private:
	void sendI1();
	void sendDhI2();

	// Called by KeyResponder::receive() when we get a response packet.
	static void gotR0(Host *h, const Endpoint &src);
	static void gotChkR1(Host *h, KeyChunkChkR1Data &r1,
				const SocketEndpoint &ep);
	static void gotDhR1(Host *h, KeyChunkDhR1Data &r1);
	static void gotDhR2(Host *h, KeyChunkDhR2Data &r2);

private slots:
	void retransmit(bool fail);
};


// This abstract base class manages the responder side of the key exchange.
class KeyResponder : public SocketReceiver
{
	friend class KeyInitiator;
	Q_OBJECT

private:
	Host *const h;

	// Cache I1 parameters of currently active checksum-armored flows,
	// for duplicate detection
	QHash<QByteArray,ChecksumArmor*> chkflows;

public:
	// Create a KeyResponder and set it listening on a particular Socket
	// for control messages with the specified magic protocol identifier.
	// The new KeyResponder becomes a child of the Socket.
	KeyResponder(Host *host, quint32 magic, QObject *parent = NULL);

	inline Host *host() { return h; }

	// Socket module calls this with control messages intended for us
	void receive(QByteArray &msg, XdrStream &rs,
			const SocketEndpoint &src);

	// Send an R0 chunk to some network address,
	// presumably a client we've discovered somehow is trying to reach us,
	// in order to punch a hole in any NATs we may be behind
	// and prod the client into (re-)sending us its I1 immediately.
	void sendR0(const Endpoint &dst);


protected:
	// KeyResponder calls this to check whether to accept a connection,
	// before actually bothering to verify the initiator's identity.
	// The default implementation always just returns true.
	virtual bool checkInitiator(const SocketEndpoint &epi,
			const QByteArray &eidi, const QByteArray &ulpi);

	// KeyResponder calls this to create a flow requested by a client.
	// The returned flow must be bound to the specified source endpoint,
	// but not yet active (started).
	// The 'ulpi' contains the information block passed by the client,
	// and the 'ulpr' block will be passed back to the client.
	// This method can return NULL to reject the incoming connection.
	virtual Flow *newFlow(const SocketEndpoint &epi,
			const QByteArray &eidi, const QByteArray &ulpi,
			QByteArray &ulpr) = 0;


private:
	void gotChkI1(KeyChunkChkI1Data &i1, const SocketEndpoint &src);

	void gotDhI1(KeyChunkDhI1Data &i1, const SocketEndpoint &src);
	void handleDhI1(quint8 dhgroup, const QByteArray &nhi,
				QByteArray &pki, const SocketEndpoint &src);
	void gotDhI2(KeyChunkDhI2Data &i2, const SocketEndpoint &src);

	static QByteArray calcDhCookie(DHKey *hk,
			const QByteArray &nr, const QByteArray &nhi,
			const Endpoint &src);

private slots:
	void checksumArmorDestroyed(QObject *obj);
};


// (endpoint, chkkey) pair for initchks hash table
typedef QPair<Endpoint, quint32> KeyEpChk;

class KeyHostState
{
	friend class KeyInitiator;

private:
	// Hash table of all currently active KeyInitiator, indexed on chkkey
	QHash<KeyEpChk, KeyInitiator*> initchks;

	// Hash table of all currently active KeyInitiator, indexed on nhi
	QHash<QByteArray, KeyInitiator*> initnhis;

	// Same, indexed on target endpoint, but allows duplicates.
	// Used for handling R0 packets during hole-punching.
	QMultiHash<Endpoint, KeyInitiator*> initeps;

public:
};


} // namespace SST

#endif	// SST_KEY_H
