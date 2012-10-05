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
/// @file C++ wrapper classes for SHA256/384/512 hash functions in OpenSSL
#ifndef SST_SHA2_H
#define SST_SHA2_H

#include <QIODevice>

#include <openssl/sha.h>

class QByteArray;


namespace SST {


struct SecureHash : public QIODevice
{
	SecureHash(QObject *parent = NULL);
	SecureHash(const SecureHash &other);

	bool isSequential() const;
	bool open(OpenMode mode);
	qint64 readData(char * data, qint64 maxSize);

	inline void init() { reset(); }
	inline void update(const void *buf, size_t size)
		{ writeData((const char*)buf, size); }
	inline void update(const QByteArray &buf)
		{ writeData(buf.constData(), buf.size()); }

	virtual int outSize() = 0;
	virtual QByteArray final() = 0;
};

struct Sha256 : public SecureHash
{
	SHA256_CTX ctx;

	Sha256(QObject *parent = NULL);
	int outSize();
	bool reset();
	qint64 writeData(const char *data, qint64 len);
	QByteArray final();

	static QByteArray hash(const void*, size_t);
	static QByteArray hash(const QByteArray &);
};

struct Sha384 : public SecureHash
{
	SHA512_CTX ctx;

	Sha384(QObject *parent = NULL);
	int outSize();
	bool reset();
	qint64 writeData(const char *data, qint64 len);
	QByteArray final();

	static QByteArray hash(const void*, size_t);
	static QByteArray hash(const QByteArray &);
};

struct Sha512 : public SecureHash
{
	SHA512_CTX ctx;

	Sha512(QObject *parent = NULL);
	int outSize();
	bool reset();
	qint64 writeData(const char *data, qint64 len);
	QByteArray final();

	static QByteArray hash(const void*, size_t);
	static QByteArray hash(const QByteArray &);
};

} // namespace SST

#endif /* SST_SHA2_H */
