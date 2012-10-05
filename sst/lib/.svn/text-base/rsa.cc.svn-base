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

#include <openssl/err.h>
#include <openssl/obj_mac.h>

#include "rsa.h"
#include "sha2.h"
#include "xdr.h"
#include "util.h"

using namespace SST;


RSAKey::RSAKey(RSA *rsa)
:	rsa(rsa)
{
}

RSAKey::RSAKey(const QByteArray &key)
{
	rsa = RSA_new();
	Q_ASSERT(rsa);

	XdrStream rds(key);
	bool hasPrivateKey;
	rds >> rsa->n >> rsa->e >> hasPrivateKey;
	if (hasPrivateKey) {
		rds >> rsa->d >> rsa->p >> rsa->q
			>> rsa->dmp1 >> rsa->dmq1 >> rsa->iqmp;
	}

	if (rds.status() == rds.Ok)
		setType(hasPrivateKey ? Private : Public);
}

RSAKey::RSAKey(int bits, unsigned e)
{
	if (bits == 0)
		bits = 1024;
	if (e == 0)
		e = 65537;
	Q_ASSERT(e % 2);	// e must be odd

	// Generate a new RSA key given those parameters
	rsa = RSA_generate_key(bits, e, NULL, NULL);
	Q_ASSERT(rsa);
	Q_ASSERT(rsa->d);
	setType(Private);
}

RSAKey::~RSAKey()
{
	if (rsa != NULL) {
		RSA_free(rsa);
		rsa = NULL;
	}
}

QByteArray RSAKey::key(bool getPrivateKey) const
{
	Q_ASSERT(type() != Invalid);

	QByteArray pk;
	XdrStream wds(&pk, QIODevice::WriteOnly);

	// Write the public part of the key
	wds << rsa->n << rsa->e;

	// Optionally write the private part
	if (getPrivateKey) {
		Q_ASSERT(type() == Private);
		wds << true << rsa->d << rsa->p << rsa->q
			<< rsa->dmp1 << rsa->dmq1 << rsa->iqmp;
	} else
		wds << false;

	Q_ASSERT(wds.status() == wds.Ok);
	return pk;
}

QByteArray RSAKey::id() const
{
	Q_ASSERT(type() != Invalid);

	QByteArray id = Sha256::hash(key());

	// Only return 160 bits of key identity information,
	// because this method's security may be limited by the SHA-1 hash
	// used in the RSA-OAEP padding process.
	id.resize(20);
	return id;
}

SecureHash *RSAKey::newHash(QObject *parent) const
{
	return new Sha256(parent);
}

QByteArray RSAKey::sign(const QByteArray &digest) const
{
	Q_ASSERT(type() != Invalid);
	Q_ASSERT(digest.size() == SHA256_DIGEST_LENGTH);

	QByteArray sig;
	unsigned siglen = RSA_size(rsa);
	sig.resize(siglen);

	//dump();

	if (!RSA_sign(NID_sha256,
			(unsigned char*)digest.data(), SHA256_DIGEST_LENGTH,
			(unsigned char*)sig.data(), &siglen, rsa))
		qFatal("RSA signing error: %s",
			ERR_error_string(ERR_get_error(), NULL));

	Q_ASSERT((int)siglen <= sig.size());
	sig.resize(siglen);

	//qDebug("Signed %s\nSignature %s",
	//	digest.toBase64().data(), sig.toBase64().data());

	return sig;
}

bool RSAKey::verify(const QByteArray &digest, const QByteArray &sig) const
{
	Q_ASSERT(type() != Invalid);
	Q_ASSERT(digest.size() == SHA256_DIGEST_LENGTH);

	//qDebug("Verifying %s\nSignature %s",
	//	digest.toBase64().data(), sig.toBase64().data());
	//dump();

	int rc = RSA_verify(NID_sha256,
			(unsigned char*)digest.data(), SHA256_DIGEST_LENGTH,
			(unsigned char*)sig.constData(), sig.size(), rsa);
	if (!rc)
		qDebug("RSA signature verification failed: %s",
			ERR_error_string(ERR_get_error(), NULL));

	return (rc == 1);
}

void RSAKey::dump() const
{
	switch (type()) {
	case Invalid:
		qDebug("Invalid RSA key");
		break;
	case Public:
	case Private:
		qDebug("%s RSA key ID %s\nn = %s\ne = %s",
			type() == Private ? "Private" : "Public",
			id().toBase64().data(),
			bn2ba(rsa->n).toBase64().data(),
			bn2ba(rsa->e).toBase64().data());
		if (type() == Private) {
			qDebug("Private key:\nd = %s\np = %s\nq = %s\n"
				"dmp1 = %s\ndmq1 = %s\niqmp = %s",
				bn2ba(rsa->d).toBase64().data(),
				bn2ba(rsa->p).toBase64().data(),
				bn2ba(rsa->q).toBase64().data(),
				bn2ba(rsa->dmp1).toBase64().data(),
				bn2ba(rsa->dmq1).toBase64().data(),
				bn2ba(rsa->iqmp).toBase64().data());
		}
		break;
	default:
		qDebug("RSA key of UNKNOWN TYPE");
		break;
	}
}

