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

#ifndef SST_AES_H
#define SST_AES_H

#include <openssl/aes.h>

class QByteArray;

namespace SST {

class AES
{
private:
	AES_KEY	aeskey;

public:
	enum Mode {
		CbcEncrypt,
		CbcDecrypt,
		CtrEncrypt = CbcEncrypt,
		CtrDecrypt = CbcEncrypt,
	};

	// Key setup
	AES &setEncryptKey(const void *key, int bits);
	AES &setEncryptKey(const QByteArray &key);

	AES &setDecryptKey(const void *key, int bits);
	AES &setDecryptKey(const QByteArray &key);

	inline AES() { }
	inline AES(const AES &other) { aeskey = other.aeskey; }
	inline AES(const QByteArray &key, Mode mode)
		{ mode == CbcEncrypt ? setEncryptKey(key)
				: setDecryptKey(key); }


	// CBC-mode encryption and decryption

	// "Raw" functions.  Cleartext buffers can be any size,
	// but encrypted data is padded to AES_BLOCK_SIZE.
	// The supplied initialization vector is updated in-place.
	void cbcEncrypt(const void *in, void *out, int size, void *ivec) const;
	void cbcDecrypt(const void *in, void *out, int size, void *ivec) const;

	// Higher-level functions - create and prepend random 16-byte IV.
	// cbcEncrypt pads input to a multiple of AES_BLOCK_SIZE;
	// cbcDecrypt does NOT strip this padding.
	QByteArray cbcEncrypt(const QByteArray &in) const;
	QByteArray cbcDecrypt(const QByteArray &in) const;


	// Encrypt/decrypt in counter mode.
	inline void ctrEncrypt(const void *in, void *out, int size,
				void *ivec, void *ctr, unsigned int &num) const
		{ AES_ctr128_encrypt((const quint8*)in, (quint8*)out, size,
				&aeskey, (quint8*)ivec, (quint8*)ctr, &num); }
	inline void ctrDecrypt(const void *in, void *out, int size,
				void *ivec, void *ctr, unsigned int &num) const
		{ AES_ctr128_encrypt((const quint8*)in, (quint8*)out, size,
				&aeskey, (quint8*)ivec, (quint8*)ctr, &num); }

	inline void ctrEncrypt(const void *in, void *out, int size,
				void *ivec) const
		{ quint8 ctr[AES_BLOCK_SIZE]; unsigned num = 0;
		  ctrEncrypt(in, out, size, ivec, ctr, num); }
	inline void ctrDecrypt(const void *in, void *out, int size,
				void *ivec) const
		{ quint8 ctr[AES_BLOCK_SIZE]; unsigned num = 0;
		  ctrDecrypt(in, out, size, ivec, ctr, num); }
};

} // namespace SST

#endif	// SST_AES_H
