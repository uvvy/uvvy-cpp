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


#include <QHash>
#include <QTime>
#include <QDateTime>
#include <QtDebug>

#include <openssl/rand.h>

#include "util.h"
#include "xdr.h"

using namespace SST;


////////// Random number generation //////////

QByteArray SST::randBytes(int size)
{
	QByteArray ba;
	ba.resize(size);
	int rc = RAND_bytes((unsigned char*)ba.data(), size);
	Q_ASSERT(rc == 1);
	return ba;
}

QByteArray SST::pseudoRandBytes(int size)
{
	QByteArray ba;
	ba.resize(size);
	int rc = RAND_pseudo_bytes((unsigned char*)ba.data(), size);
	Q_ASSERT(rc >= 0);
	return ba;
}


////////// BIGNUM conversions //////////

BIGNUM *SST::ba2bn(const QByteArray &ba, BIGNUM *ret)
{
	return BN_bin2bn((const unsigned char*)ba.data(), ba.size(), ret);
}

QByteArray SST::bn2ba(const BIGNUM *bn)
{
	Q_ASSERT(bn != NULL);
	QByteArray ba;
	ba.resize(BN_num_bytes(bn));
	BN_bn2bin(bn, (unsigned char*)ba.data());
	return ba;
}

XdrStream &SST::operator<<(XdrStream &xs, BIGNUM *bn)
{
	return xs << bn2ba(bn);
}

XdrStream &SST::operator>>(XdrStream &xs, BIGNUM *&bn)
{
	QByteArray ba;
	xs >> ba;
	bn = ba2bn(ba, bn);
	return xs;
}


////////// Endpoint //////////

Endpoint::Endpoint()
:	port(0)
{
}

Endpoint::Endpoint(quint32 ip4addr, quint16 port)
:	addr(ip4addr), port(port)
{
}

Endpoint::Endpoint(const QHostAddress &addr, quint16 port)
:	addr(addr), port(port)
{
}

QString Endpoint::toString() const
{
	return addr.toString() + ":" + QString::number(port);
}

void Endpoint::encode(XdrStream &xs) const
{
	switch (addr.protocol()) {
	case QAbstractSocket::IPv4Protocol:
		xs << (qint32)IPv4 << addr.toIPv4Address();
		break;
	case QAbstractSocket::IPv6Protocol: {
		Q_IPV6ADDR a6 = addr.toIPv6Address();
		Q_ASSERT(sizeof(a6) == 16);
		xs << (qint32)IPv6;
		xs.writeRawData(&a6[0], 16);
		break; }
	default:
		xs.setStatus(xs.IOError);	// XX ??
		break;
	}
	xs << port;
}

void Endpoint::decode(XdrStream &xs)
{
	qint32 type;
	xs >> type;
	switch ((Endpoint::Type)type) {
	case IPv4: {
		quint32 addr4;
		xs >> addr4;
		addr.setAddress(addr4);
		break; }
	case IPv6: {
		Q_IPV6ADDR addr6;
		xs.readRawData(&addr6[0], 16);
		addr.setAddress(addr6);
		break; }
	default:
		xs.setStatus(xs.IOError);	// XX ??
		break;
	}
	xs >> port;
}

#if QT_VERSION < 0x040200	// provided by Qt as of version 4.2.x
uint qHash(const QHostAddress &addr)
{
	switch (addr.protocol()) {
	case QAbstractSocket::IPv4Protocol:
		return qHash(addr.toIPv4Address());
	case QAbstractSocket::IPv6Protocol: {
		Q_IPV6ADDR a6 = addr.toIPv6Address();
		Q_ASSERT(sizeof(a6) == 16);
		return qHash(QByteArray((const char*)&a6[0], 16));
		}
	default:
		return qHash(addr.toString());
	}
}
#endif

uint qHash(const Endpoint &ep)
{
	return qHash(ep.addr) + qHash(ep.port);
}


////////// Byte count stringifier //////////

QString bytesNumber(qint64 size)
{
	static const qint64 KB = 1024;
	static const qint64 MB = 1024 * KB;
	static const qint64 GB = 1024 * MB;
	static const qint64 TB = 1024 * GB;

	if (size >= TB) {
		return QObject::tr("%0 TB")
			.arg((double)size / TB, 0, 'f', 1);
	} else if (size >= GB) {
		return QObject::tr("%0 GB")
			.arg((double)size / GB, 0, 'f', 1);
	} else if (size >= MB) {
		return QObject::tr("%0 MB")
			.arg((double)size / MB, 0, 'f', 1);
	} else if (size >= KB) {
		return QObject::tr("%0 KB")
			.arg((double)size / KB, 0, 'f', 1);
	} else
		return QString::number(size);
}


