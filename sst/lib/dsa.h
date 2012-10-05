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
#ifndef SST_DSA_H
#define SST_DSA_H

#include <openssl/dsa.h>

#include "sign.h"

namespace SST {

class DSAKey : public SignKey
{
	DSA *dsa;

	DSAKey(DSA *dsa);

public:
	DSAKey(const QByteArray &key);
	DSAKey(int bits);
	~DSAKey();

	QByteArray id() const;
	QByteArray key(bool getPrivateKey = false) const;

	SecureHash *newHash(QObject *parent = NULL) const;

	QByteArray sign(const QByteArray &digest) const;
	bool verify(const QByteArray &digest, const QByteArray &sig) const;
};

} // namespace SST

#endif	// SST_DSA_H
