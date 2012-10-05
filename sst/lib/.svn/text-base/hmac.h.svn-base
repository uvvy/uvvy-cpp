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
/*** Simple HMAC-SHA256-128 implementation ***/

#ifndef SST_HMAC_H
#define SST_HMAC_H

#include "sha2.h"

namespace SST {


// Length of symmetric key material for HMAC-SHA-256-128
#define HMACKEYLEN	(256/8)

// We use SHA-256 hashes truncated to 128 bits for HMAC generation
#define HMACLEN		(128/8)


typedef SHA256_CTX hmac_ctx;


// Low-level functions
void hmac_init(hmac_ctx *ctx, const uint8_t *hkey);
void hmac_update(hmac_ctx *ctx, const void *data, size_t len);
void hmac_final(hmac_ctx *ctx, const uint8_t *hkey,
			uint8_t *outbuf, unsigned outlen);


class HMAC : public SecureHash
{
private:
	hmac_ctx ictx, octx;

	// Not usable in this implementation of IODevice
	bool reset();

public:
	HMAC(const QByteArray &key);
	HMAC(const HMAC &other);

	// Write data to the HMAC
	qint64 writeData(const char *data, qint64 len);

	// Produce the output, with the default (truncated) size of HMACLEN.
	int outSize();
	QByteArray final();

	// Produce the output with a specified size.
	QByteArray final(int macsize);

	// Update with the contents of a message
	// and append a MAC to the message.
	inline void finalAppend(QByteArray &msg, int macsize = HMACLEN)
		{ update(msg); msg += final(macsize); }

	// Update with the contents of a message minus its MAC trailer,
	// verify the MAC trailer, and cut it from the message.
	bool finalVerify(QByteArray &msg, int macsize = HMACLEN); 


	///// Convenience functions for handling contiguous messages /////
	// These functions are const because they copy the HMAC state
	// to temporary storage before computing their result.

	// Compute an HMAC check over a message
	QByteArray calc(const void *msg, int msgsize,
				int macsize = HMACLEN) const;
	inline QByteArray calc(QByteArray &msg, int macsize = HMACLEN) const
		{ return calc(msg.data(), msg.size(), macsize); }

	// Compute and append HMAC check field to a message.
	void calcAppend(QByteArray &msg, int macsize = HMACLEN) const;

	// Strip and check the HMAC check field on receive.
	bool calcVerify(QByteArray &msg, int macsize = HMACLEN) const;
};

} // namespace SST

#endif	// SST_HMAC_H
