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


#include <QtEndian>
#include <QByteArray>

#include "sha2.h"

using namespace SST;


////////// SecureHash //////////

SecureHash::SecureHash(QObject *parent)
:	QIODevice(parent)
{
	open(WriteOnly);
}

SecureHash::SecureHash(const SecureHash &other)
:	QIODevice(other.parent())
{
}

bool SecureHash::open(OpenMode mode)
{
	Q_ASSERT(mode == WriteOnly);
	return QIODevice::open(mode);
}

bool SecureHash::isSequential() const
{
	return true;
}

qint64 SecureHash::readData(char *, qint64)
{
	setErrorString("Can't read from hash device");
	return -1;
}


////////// Sha256 //////////

Sha256::Sha256(QObject *parent)
:	SecureHash(parent)
{
	reset();
}

int Sha256::outSize()
{
	return SHA256_DIGEST_LENGTH;
}

bool Sha256::reset()
{
	SHA256_Init(&ctx);
	return true;
}

qint64 Sha256::writeData(const char *buf, qint64 size)
{
	SHA256_Update(&ctx, (const uint8_t*)buf, size);
	return size;
}

QByteArray Sha256::final()
{
	QByteArray hash;
	hash.resize(SHA256_DIGEST_LENGTH);
	SHA256_Final((unsigned char*)hash.data(), &ctx);
	return hash;
}

QByteArray Sha256::hash(const void *buf, size_t size)
{
	Sha256 c;
	c.update(buf, size);
	return c.final();
}

QByteArray Sha256::hash(const QByteArray &buf)
{
	return hash(buf.data(), buf.size());
}


////////// SHA384 //////////

Sha384::Sha384(QObject *parent)
:	SecureHash(parent)
{
	reset();
}

int Sha384::outSize()
{
	return SHA384_DIGEST_LENGTH;
}

bool Sha384::reset()
{
	SHA384_Init(&ctx);
	return true;
}

qint64 Sha384::writeData(const char *buf, qint64 size)
{
	SHA384_Update(&ctx, (const uint8_t*)buf, size);
	return size;
}

QByteArray Sha384::final()
{
	QByteArray hash;
	hash.resize(SHA384_DIGEST_LENGTH);
	SHA384_Final((unsigned char*)hash.data(), &ctx);
	return hash;
}

QByteArray Sha384::hash(const void *buf, size_t size)
{
	Sha384 c;
	c.update(buf, size);
	return c.final();
}

QByteArray Sha384::hash(const QByteArray &buf)
{
	return hash(buf.data(), buf.size());
}


////////// SHA512 //////////

Sha512::Sha512(QObject *parent)
:	SecureHash(parent)
{
	reset();
}

int Sha512::outSize()
{
	return SHA512_DIGEST_LENGTH;
}

bool Sha512::reset()
{
	SHA512_Init(&ctx);
	return true;
}

qint64 Sha512::writeData(const char *buf, qint64 size)
{
	SHA512_Update(&ctx, (const uint8_t*)buf, size);
	return size;
}

QByteArray Sha512::final()
{
	QByteArray hash;
	hash.resize(SHA512_DIGEST_LENGTH);
	SHA512_Final((unsigned char*)hash.data(), &ctx);
	return hash;
}

QByteArray Sha512::hash(const void *buf, size_t size)
{
	Sha512 c;
	c.update(buf, size);
	return c.final();
}

QByteArray Sha512::hash(const QByteArray &buf)
{
	return hash(buf.data(), buf.size());
}


