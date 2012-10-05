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


#include <string.h>

#include <QDataStream>
#include <QHostAddress>
#include <QSettings>
#include <QtDebug>

#include "ident.h"
#include "dsa.h"
#include "util.h"
#include "sha2.h"
#include "xdr.h"
#include "rsa.h"
#include "dsa.h"

using namespace SST;


////////// IdentData //////////

IdentData::IdentData(const IdentData &other)
:	QSharedData(other),
	id(other.id),
	k(NULL)
{
	if (other.k) {
		//qDebug("IdentData: copying key");

		// Copy the key by serializing and deserializing it.
		bool isPrivate = other.k->type() == SignKey::Private;
		bool success = setKey(other.k->key(isPrivate));
		Q_ASSERT(success);
	}
}

IdentData::~IdentData()
{
	clearKey();
}

bool IdentData::setKey(const QByteArray &key)
{
	// Clear any previous decoded key
	clearKey();

	// Decode the new key
	Q_ASSERT(!id.isEmpty());
	Ident::Scheme sch = (Ident::Scheme)((quint8)id[0] >> 2);
	switch (sch) {
	case Ident::DSA160:	k = new DSAKey(key); break;
	case Ident::RSA160:	k = new RSAKey(key); break;
	default:
		qDebug("Unknown identity scheme %d", sch);
		return false;		// Unknown scheme
	}

	// Make sure the scheme-specific decode succeeded
	if (k->type() == k->Invalid) {
		bad:
		clearKey();
		return false;			// Invalid encoded key
	}

	// Verify that the supplied key actually matches the ID we have.
	// This is a crucial step for security!
	QByteArray kid = k->id();
	Q_ASSERT(!kid.isEmpty());
	kid[0] = sch << 2;
	if (kid != id)
		goto bad;

	Q_ASSERT(k->type() != k->Invalid);
	return true;
}

void IdentData::clearKey()
{
	if (k) {
		delete k;
		k = NULL;
	}
}


////////// Ident //////////

static Ident nullIdent = Ident(QByteArray());

Ident::Ident()
:	d(nullIdent.d)
{
}

Ident::Ident(const QByteArray &id)
:	d(new IdentData(id))
{
}

Ident::Ident(const QByteArray &id, const QByteArray &key)
:	d(new IdentData(id))
{
	setKey(key);
}

void Ident::setID(const QByteArray &id)
{
	d->id = id;
	d->clearKey();
}

bool Ident::setKey(const QByteArray &key)
{
	return d->setKey(key);
}

QByteArray Ident::hash(const void *data, int len) const
{
	SecureHash *sh = newHash(NULL);
	sh->update(data, len);
	QByteArray h = sh->final();
	delete sh;
	return h;
}

Ident Ident::generate(Scheme sch, int bits)
{
	// Generate a key using the appropriate scheme
	SignKey *k;
	switch (sch) {
	case DSA160:
		k = new DSAKey(bits);
		break;
	case RSA160:
		k = new RSAKey(bits);
		break;
	default:
		Q_ASSERT(0);
	}

	// Find the corresponding ID
	QByteArray id = k->id();
	id[0] = sch << 2;

	// Create the Ident
	Ident ident(id);
	ident.d->k = k;
	return ident;
}

Ident Ident::fromMacAddress(const QByteArray &addr)
{
	Q_ASSERT(addr.size() == 6);

	QByteArray id(1, (char)MAC << 2);
	id += addr;
	return Ident(id);
}

QByteArray Ident::macAddress()
{
	if (scheme() != MAC || d->id.size() != 1+6)
		return QByteArray();
	return d->id.mid(1);
}

Ident Ident::fromIpAddress(const QHostAddress &addr, quint16 port)
{
	QByteArray id;

	// Encode the scheme number and address
	switch (addr.protocol()) {
	case QAbstractSocket::IPv4Protocol: {
		id.append((char)IP*4 + 0);	// IPv4 address subscheme
		quint32 a4 = htonl(addr.toIPv4Address());
		id.append(QByteArray((char*)&a4, 4));
		Q_ASSERT(id.size() == 1+4);
		break; }
	case QAbstractSocket::IPv6Protocol: {
		id.append((char)IP*4 + 1);	// IPv6 address subscheme
		Q_IPV6ADDR a6 = addr.toIPv6Address();
		id.append(QByteArray((const char*)&a6[0], 16));
		Q_ASSERT(id.size() == 1+16);
		break; }
	default:
		// Unknown protocol.
		return Ident();
	}

	// Encode the port number, if any
	if (port != 0) {
		quint16 portn = htons(port);
		id.append(QByteArray((const char*)&portn, 2));
	}

	return Ident(id);
}

QHostAddress Ident::ipAddress(quint16 *out_port)
{
	if (out_port)
		*out_port = 0;
	if (d->id.isEmpty())
		return QHostAddress();

	// Decode the scheme, subscheme, and address
	QHostAddress addr;
	int idx;
	switch (d->id[0]) {
	case ((quint8)IP*4 + 0): {	// IPv4 address subscheme
		if (d->id.size() < 1+4)
			return QHostAddress();
		quint32 a4;
		memcpy(&a4, d->id.data() + 1, 4);
		addr.setAddress(ntohl(a4));
		idx = 1+4;
		break; }
	case ((quint8)IP*4 + 1): {	// IPv6 address subscheme
		if (d->id.size() < 1+16)
			return QHostAddress();
		Q_IPV6ADDR a6;
		memcpy(&a6, d->id.data() + 1, 16);
		addr.setAddress(a6);
		idx = 1+16;
		break; }
	default:
		return QHostAddress();
	}

	// Decode the optional port number
	quint16 portn = 0;
	if (d->id.size() >= idx+2)
		memcpy(&portn, d->id.data() + idx, 2);
	if (out_port)
		*out_port = ntohs(portn);

	return addr;
}

quint16 Ident::ipPort()
{
	quint16 port;
	ipAddress(&port);
	return port;
}

Ident Ident::fromEndpoint(const Endpoint &ep)
{
	return fromIpAddress(ep.addr, ep.port);
}

Endpoint Ident::endpoint()
{
	Endpoint ep;
	ep.addr = ipAddress();
	ep.port = ipPort();
	return ep;
}


////////// IdentHostState //////////

Ident IdentHostState::hostIdent(bool create)
{
	if (!create || hid.havePrivateKey())
		return hid;

	// Generate and return a new key pair
	return hid = Ident::generate();
}

void IdentHostState::setHostIdent(const Ident &ident)
{
	if (!ident.havePrivateKey())
		qWarning("Ident: using a host identity with no private key!");
	hid = ident;
}

void IdentHostState::initHostIdent(QSettings *settings)
{
	if (hid.havePrivateKey())
		return;		// Already initialized
	if (!settings)
		return (void)hostIdent(true);	// No persistence

	// Find and decode the host's existing key, if any.
	QByteArray id = settings->value("id").toByteArray();
	QByteArray key = settings->value("key").toByteArray();
	if (!id.isEmpty() && !key.isEmpty()) {

		hid.setID(id);
		if (hid.setKey(key) && hid.havePrivateKey())
			return;		// Success

		qWarning("Ident: invalid host identity in settings: "
			"generating new identity.");
	}

	// Generate a new key pair
	hid = Ident::generate();

	// Save it in our host settings
	settings->setValue("id", hid.id());
	settings->setValue("key", hid.key(true));
	settings->sync();
}

