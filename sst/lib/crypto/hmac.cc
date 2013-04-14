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
#include <assert.h>

#include <QByteArray>

#include "hmac.h"
#include "sha2.h"

using namespace SST;


/*** Simple HMAC-SHA256-128 implementation ***/

#define IPAD	0x36
#define OPAD	0x5c

// Helper function for HMAC-SHA256-128
static void hmac_initpad(hmac_ctx *ctx, const uint8_t *hkey, uint8_t pad)
{
	// Compute the HMAC keyed pre- or post-hash block (see RFC 2104)
	uint8_t padbuf[SHA256_CBLOCK];
	memset(padbuf, pad, SHA256_CBLOCK);
	for (unsigned i = 0; i < HMACKEYLEN; i++)
		padbuf[i] ^= hkey[i];

	// Initialize and precondition the provided SHA256_CTX 
	SHA256_Init(ctx);
	SHA256_Update(ctx, padbuf, SHA256_CBLOCK);
}

// Initialize an hmac_ctx for keyed HMAC-SHA256-128 computation.
void SST::hmac_init(hmac_ctx *ctx, const uint8_t *hkey)
{
	hmac_initpad(ctx, hkey, IPAD);
}

// Update a SHA256_CTX with data to hash
void SST::hmac_update(hmac_ctx *ctx, const void *data, size_t len)
{
	SHA256_Update(ctx, (uint8_t*)data, len);
}

// Finalize an hmac_ctx and compute the final 128-bit MAC
void SST::hmac_final(hmac_ctx *ctx, const uint8_t *hkey,
			uint8_t *outbuf, unsigned outlen)
{
	// Compute the pre-hash from the provided context
	uint8_t prehash[SHA256_DIGEST_LENGTH];
	SHA256_Final(prehash, ctx);

	// Compute the post-hash from the prehash and the key
	hmac_initpad(ctx, hkey, OPAD);
	SHA256_Update(ctx, prehash, SHA256_DIGEST_LENGTH);
	uint8_t posthash[SHA256_DIGEST_LENGTH];
	SHA256_Final(posthash, ctx);

	// Truncate the resulting hash to 128 bits
	assert(outlen <= SHA256_DIGEST_LENGTH);
	memcpy(outbuf, posthash, outlen);
}


// High-level routines

HMAC::HMAC(const QByteArray &key)
{
	Q_ASSERT(key.size() == HMACKEYLEN);	// XX could easily be flexible
	hmac_initpad(&ictx, (const uint8_t*)key.constData(), IPAD);
	hmac_initpad(&octx, (const uint8_t*)key.constData(), OPAD);
}

HMAC::HMAC(const HMAC &other)
:	SecureHash(other),
	ictx(other.ictx),
	octx(other.octx)
{
}

bool HMAC::reset()
{
	qFatal("HMAC: can't reset, shouldn't have tried");
	return false;
}

int HMAC::outSize()
{
	// The default output size, as returned by final() with no argument
	return HMACLEN;
}

qint64 HMAC::writeData(const char *data, qint64 len)
{
	SHA256_Update(&ictx, (const quint8*)data, len);
	return len;
}

QByteArray HMAC::final(int macsize)
{
	uint8_t ihash[SHA256_DIGEST_LENGTH];
	SHA256_Final(ihash, &ictx);

	SHA256_Update(&octx, ihash, SHA256_DIGEST_LENGTH);
	uint8_t ohash[SHA256_DIGEST_LENGTH];
	SHA256_Final(ohash, &octx);

	Q_ASSERT(macsize >= HMACLEN && macsize <= SHA256_DIGEST_LENGTH);
	return QByteArray((const char*)ohash, macsize);
}

QByteArray HMAC::final()
{
	return final(HMACLEN);
}

bool HMAC::finalVerify(QByteArray &msg, int macsize)
{
	int msgsize = msg.size() - macsize;
	if (msgsize < 0)
		return false;

	// Calculate and verify the MAC field
	update(msg.data(), msgsize);
	QByteArray mac = final(macsize);
	bool rc = msg.endsWith(mac);

	// Chop off the MAC
	msg.resize(msgsize);

	return rc;
}

QByteArray HMAC::calc(const void *msg, int msgsize, int macsize) const
{
	HMAC hmcopy(*this);
	hmcopy.update(msg, msgsize);
	return hmcopy.final(macsize);
}

void HMAC::calcAppend(QByteArray &msg, int macsize) const
{
	HMAC hmcopy(*this);
	return hmcopy.finalAppend(msg, macsize);
}

bool HMAC::calcVerify(QByteArray &msg, int macsize) const
{
	HMAC hmcopy(*this);
	return hmcopy.finalVerify(msg, macsize);
}

