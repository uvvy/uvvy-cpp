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
#ifndef SST_IDENT_H
#define SST_IDENT_H

#include <QByteArray>
#include <QSharedData>

#include <openssl/dsa.h>
#include <openssl/rsa.h>

#include "sign.h"

class QHostAddress;
class QSettings;


namespace SST {

class SecureHash;
class Ident;
class Endpoint;


class IdentData : public QSharedData
{
	friend class Ident;

	QByteArray id;
	SignKey *k;

public:
	inline IdentData(const QByteArray id) : id(id), k(NULL) { }
	IdentData(const IdentData &other);
	~IdentData();

	bool setKey(const QByteArray &key);
	void clearKey();
};

/** Represents an endpoint identifier
 * and optionally an associated cryptographic signing key.
 */
class Ident
{
	QSharedDataPointer<IdentData> d;

public:
	/** Endpoint identifier scheme numbers.
	 * The scheme number occupies the top 6 bits in any EID,
	 * making the EID's scheme easily recognizable
	 * via the first character in its Base64 representation. */
	enum Scheme {
		NoScheme = 0,	///< Reserved for the "null" Ident.

		// Non-cryptographic legacy address schemes
		MAC	= 1,	///< IEEE MAC address
		IP	= 2,	///< IP address with optional port

		// Cryptographic identity schemes
		DSA160	= 10,	///< DSA with SHA-256, yielding 160-bit IDs
		RSA160	= 11,	///< RSA with SHA-256, yielding 160-bit IDs
	};

	/// Create a null Ident.
	Ident();

	/** Create an Ident with a given binary identifier.
	 * @param id the binary identifier.
 	 */
	Ident(const QByteArray &id);

	/** Create an Ident with a binary identifier and corresponding key.
	 * @param id the binary identifier.
	 * @param key the binary representation of the key
	 *	associated with the identifier.
	 */
	Ident(const QByteArray &id, const QByteArray &key);


	/** Get this identifier's short binary ID.
	 * @return the binary identifier, as a QByteArray. */
	inline QByteArray id() const { return d->id; }

	/** Set the Ident's short binary ID.
	 * Clears any associated key information.
	 * @param id the binary identifier.
	 */
	void setID(const QByteArray &id);

	/** Check for the distinguished "null identity".
	 * @return true if this is a null Ident. */
	inline bool isNull() { return d->id.isEmpty(); }

	/** Determine the scheme number this ID uses.
	 * @return the scheme number. */
	inline Scheme scheme() const {
		return d->id.isEmpty() ? NoScheme
			: (Scheme)(d->id.at(0) >> 2); }

	/** Determine whether this identifier contains an associated key
	 * usable for signature verification.
	 * @return true if this Ident contains a public key. */
	inline bool haveKey() const
		{ return d->k && d->k->type() != SignKey::Invalid; }

	/** Determine whether this identifier contains a private key
	 * usable for both signing and verification.
	 * @return true if this Ident contains a private key. */
	inline bool havePrivateKey() const
		{ return d->k && d->k->type() == SignKey::Private; }

	/** Get this Ident's binary-encoded public or private key.
	 * @param getPrivateKey true to obtain the complete
	 *		public/private key pair if available,
	 *		false to obtain only the public key.
	 * @return the key encoded into a QByteArray.
	 */
	inline QByteArray key(bool getPrivateKey = false) const
		{ return d->k->key(getPrivateKey); }

	/** Set the public or private key associated with this Ident.
	 * @param key the binary-encoded public or private key.
	 * @return true if the encoded key was recognized, valid,
	 *	and the correct key for this identifier.
	 */
	bool setKey(const QByteArray &key);

	/** Create a new SecureHash object suitable for hashing messages
	 * to be signed using this identity's private key.
	 * @param parent the optional parent for the new SecureHash object.
	 * @return the new SecureHash object.
	 * 	The caller must delete it when done
	 *	(or allow it to be deleted by its parent). */
	inline SecureHash *newHash(QObject *parent = NULL) const
		{ return d->k->newHash(parent); }

	/** Hash a block of data using this Ident scheme's hash function.
	 * This is just a convenience function based on newHash().
	 * @param data a pointer to the data to hash.
	 * @param len the number of bytes to hash.
	 * @return the resulting hash, in a QByteArray.
	 * @see newHash
	 */
	QByteArray hash(const void *data, int len) const;

	/** Hash a QByteArray using this Ident scheme's hash function.
	 * This is just a convenience function based on newHash().
	 * @param data the QByteArray to hash.
	 * @return the resulting hash, in a QByteArray.
	 * @see newHash
	 */
	inline QByteArray hash(const QByteArray &data) const
		{ return hash(data.constData(), data.size()); }

	/** Sign a message.
	 * This Ident must contain a valid private key.
	 * @param digest the hash digest of the message to be signed.
	 * @return the resulting signature, in a QByteArray.
	 */
	inline QByteArray sign(const QByteArray &digest) const
		{ return d->k->sign(digest); }

	/** Verify a signature.
	 * This ident must contain a valid public key.
	 * @param digest the hash digest of the signed message.
	 * @param sig the signature to be verified.
	 * @return true if signature verification succeeds.
	 */
	inline bool verify(const QByteArray &digest,
				const QByteArray &sig) const
		{ return d->k->verify(digest, sig); }

	/** Generate a new Ident with a unique private key,
	 * using reasonable default parameters.
	 * @param sch the signing scheme to use.
	 * @param bits the desired key strength in bits,
	 *	or 0 to use the selected scheme's default.
	 * @return the generated Ident.
	 */
	static Ident generate(Scheme sch = RSA160, int bits = 0);


	/** Create an Ident representing a non-cryptographic IEEE MAC address.
	 * Non-cryptographic identifiers cannot have signing keys.
	 * @param addr the 6-byte MAC address.
	 * @return the resulting Ident.
	 */
	static Ident fromMacAddress(const QByteArray &addr);

	/** Extract the IEEE MAC address in an identifier with the MAC scheme.
	 * @return the 6-byte MAC address. */
	QByteArray macAddress();


	/** Create an Ident representing a non-cryptographic IP address.
	 * Non-cryptographic identifiers cannot have signing keys.
	 * @param addr the IP address.
	 * @param port an optional transport-layer port number.
	 */
	static Ident fromIpAddress(const QHostAddress &addr, quint16 port = 0);

	/** Extract the host address part of an identifier in the IP scheme.
	 * @param port if non-NULL, location to receive optional port number.
	 * @return an IPv4 or an IPv6 address. */
	QHostAddress ipAddress(quint16 *out_port = NULL);

	/** Extract the port number part of an identifier in the IP scheme.
	 * @return the 16-bit port number, 0 if the EID contains no port. */
	quint16 ipPort();

	/** Create an Ident representing a non-cryptographic endpoint
	 * (IP address and port number pair).
	 */
	static Ident fromEndpoint(const Endpoint &ep);

	/** Extract the endpoint (IP address and port pair)
	 * from an EID that describes an endpoint. */
	Endpoint endpoint();
};


/** Per-host state for the Ident module. */
class IdentHostState
{
	Ident hid;


public:
	/** Obtain our own global primary host identity and private key.
	 * @param create If true (the default), generates a new identity
	 *	and private key if one isn't already set.
	 *	If false, just returns a null Ident in that case.
	 * @return the primary host identity. */
	Ident hostIdent(bool create = true);

	/** Set our primary host identity.
	 * @param ident the new host identity to set,
	 *		which should include a private key. */
	void setHostIdent(const Ident &ident);

	/** Initialize our primary host identity
	 * using a QSettings registry for persistence.
	 * Looks for the tags 'id' and 'key' in the provided @a settings,
	 * and if they contain a valid host identity and private key,
	 * uses them to set the primary host identity.
	 * Otherwise, generates a new primary host identity
	 * and encodes them into the provided @a settings
	 * for future invocations of the application.
	 *
	 * This function does nothing if the primary host identity
	 * is already initialized and contains a valid private key.
	 *
	 * @param settings the settings registry to use for persistence. */
	void initHostIdent(QSettings *settings);
};


} // namespace SST

#endif	// SST_IDENT_H
