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
#ifndef SST_DH_H
#define SST_DH_H

#include <QObject>
#include <QHash>

#include <openssl/dh.h>

#include "timer.h"


namespace SST {


#define HOSTKEY_TIMEOUT		(60*60)	// Host key timeout in seconds - 1 hr

#define KEYGROUP_JFDH_1024	0x01
#define KEYGROUP_JFDH_2048	0x02
#define KEYGROUP_JFDH_3072	0x03
#define KEYGROUP_JFDH_MAX	0x03
#define KEYGROUP_JFDH_DEFAULT	KEYGROUP_JFDH_1024

class Host;

class DHKey : public QObject
{
	friend class DHHostState;
	friend class KeyInitiator;	// XX
	friend class KeyResponder;	// XX

	Q_OBJECT

private:
	Host *const host;	///< Host to which this key is attached
	Timer exptimer;		///< DH master key expiration timer

	quint8 dhgroup;
	DH *dh;
	QByteArray pubkey;
	quint8 hkr[256/8];	// HMAC key for responder's challenge

	// Hash table of cached R2 responses made using this key,
	// for replay protection.
	QHash<QByteArray, QByteArray> r2cache;


public:
	// Compute a shared master secret from our private key and otherPubKey.
	QByteArray calcKey(const QByteArray &otherPubKey);

private:
	DHKey(Host *host, quint8 dhgroup, DH *dh,
		int timeoutSecs = HOSTKEY_TIMEOUT);

private slots:
	void timeout();
};


// Per-host state for the DH key agreement module.
class DHHostState
{
	friend class DHKey;

private:
	DHKey *dhkeys[KEYGROUP_JFDH_MAX];

	DHKey *gen(quint8 dhgroup, DH *(*groupfunc)());

public:
	DHHostState();
	virtual ~DHHostState();

	DHKey *getDHKey(quint8 dhgroup);

	virtual Host *host() = 0;
};


QDataStream &operator<<(QDataStream &ds, DH *dh);
QDataStream &operator>>(QDataStream &ds, DH *dh);


} // namespace SST

#endif	// SST_DH_H
