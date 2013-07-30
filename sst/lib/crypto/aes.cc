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


#include <QByteArray>

#include "aes.h"
#include "util.h"

using namespace SST;


AES &
AES::setEncryptKey(const void *key, int bits)
{
	int rc = AES_set_encrypt_key((const unsigned char*)key, bits, &aeskey);
	Q_ASSERT(rc == 0);
	return *this;
}

AES &
AES::setEncryptKey(const QByteArray &key)
{
	int keysize = key.size();
	Q_ASSERT(keysize == 128/8 || keysize == 192/8 || keysize == 256/8);
	setEncryptKey(key.constData(), keysize*8);
	return *this;
}

AES &
AES::setDecryptKey(const void *key, int bits)
{
	int rc = AES_set_decrypt_key((const unsigned char*)key, bits, &aeskey);
	Q_ASSERT(rc == 0);
	return *this;
}

AES &
AES::setDecryptKey(const QByteArray &key)
{
	int keysize = key.size();
	Q_ASSERT(keysize == 128/8 || keysize == 192/8 || keysize == 256/8);
	setDecryptKey(key.constData(), keysize*8);
	return *this;
}

inline void AES::cbcEncrypt(const void *in, void *out, int size, void *ivec)
	const
{
	AES_cbc_encrypt((const unsigned char*)in, (unsigned char*)out,
			size, &aeskey, (unsigned char*)ivec, AES_ENCRYPT);
}

QByteArray AES::cbcEncrypt(const QByteArray &in) const
{
	int size = in.size();
	int padsize = (size + AES_BLOCK_SIZE-1) & ~(AES_BLOCK_SIZE-1);

	QByteArray ivec = randBytes(AES_BLOCK_SIZE);
	QByteArray out = ivec;
	out.resize(AES_BLOCK_SIZE + padsize);
	cbcEncrypt(in.constData(), out.data() + AES_BLOCK_SIZE,
			size, ivec.data());
	return out;
}

inline void AES::cbcDecrypt(const void *in, void *out, int size, void *ivec)
	const
{
	AES_cbc_encrypt((const unsigned char*)in, (unsigned char*)out,
			size, &aeskey, (unsigned char*)ivec, AES_DECRYPT);
}

QByteArray AES::cbcDecrypt(const QByteArray &in) const
{
	int padsize = in.size() - AES_BLOCK_SIZE;
	if (padsize <= 0 || (padsize % AES_BLOCK_SIZE) != 0)
		return QByteArray();

	QByteArray ivec = in.left(AES_BLOCK_SIZE);
	QByteArray out;
	out.resize(padsize);
	cbcDecrypt(in.constData() + AES_BLOCK_SIZE,
			out.data(), padsize, ivec.data());
	return out;
}

