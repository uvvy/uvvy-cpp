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
#ifndef SST_REG_H
#define SST_REG_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QFlags>

namespace SST {

class XdrStream;
class Endpoint;


#define REGSERVER_DEFAULT_PORT	8662

// Control chunk magic value for the Netsteria registration protocol.
// The upper byte is zero to distinguish control packets from flow packets.
// 'Nrs': Netsteria registration server
#define REG_MAGIC	(quint32)0x004e7273

#define REG_REQUEST		0x100	// Client-to-server request
#define REG_RESPONSE		0x200	// Server-to-client response
#define REG_NOTIFY		0x300	// Server-to-client async callback

#define REG_INSERT1		0x00	// Insert entry - preliminary request
#define REG_INSERT2		0x01	// Insert entry - authenticated request
#define REG_LOOKUP		0x02	// Lookup host by ID, optionally notify
#define REG_SEARCH		0x03	// Search entry by keyword


// A RegInfo object represents a client-specified block of information
// about itself to be made publicly available to other clients
// through a registration server.
class RegInfo
{
public:
	// Attribute tags - will grow over time.
	// The upper 16 bits are for attribute property flags.
	enum AttrFlag {
		Invalid		= 0x00000000,	// Invalid attribute

		// Flags indicating useful properties of certain tags
		SearchFlag	= 0x00010000,	// Search-worthy text (UTF-8)

		// Specific binary tags useful for rendezvous
		Endpoints	= 0x00000001,	// Private addrs for hole punch

		// UTF-8 string tags representing advertised information
		HostName	= 0x00010001,	// Name of host (machine)
		OwnerName	= 0x00010002,	// Name of owner (human)
		City		= 0x00010003,	// Metropolitan area
		Region		= 0x00010004,	// State or other locality
		Country		= 0x00010005,
	};
	Q_DECLARE_FLAGS(Attr, AttrFlag)

private:
	QHash<Attr, QByteArray> at;


public:
	// Basic attribute management methods
	inline QByteArray attr(Attr tag) const
		{ return at.value(tag); }
	inline void setAttr(Attr tag, const QByteArray &value)
		{ at.insert(tag, value); }
	inline void remove(Attr tag)
		{ setAttr(tag, QByteArray()); }
	inline bool isEmpty() const { return at.isEmpty(); }
	inline QList<Attr> tags() const { return at.keys(); }

	// String attribute get/set
	inline QString string(Attr tag) const
		{ return QString::fromUtf8(at.value(tag)); }
	inline void setString(Attr tag, const QString &value)
		{ at.insert(tag, value.toUtf8()); }

	// Type-specific methods for individual attributes
	inline QString hostName() const { return string(HostName); }
	inline QString ownerName() const { return string(OwnerName); }
	inline QString city() const { return string(City); }
	inline QString region() const { return string(Region); }
	inline QString country() const { return string(Country); }

	inline void setHostName(const QString &str)
		{ setString(HostName, str); }
	inline void setOwnerName(const QString &str)
		{ setString(OwnerName, str); }
	inline void setCity(const QString &str)
		{ setString(City, str); }
	inline void setRegion(const QString &str)
		{ setString(Region, str); }
	inline void setCountry(const QString &str)
		{ setString(Country, str); }

	// Advertised private endpoints
	QList<Endpoint> endpoints() const;
	void setEndpoints(const QList<Endpoint> &endpoints);

	// Return all the words appearing in all searchable string attrs.
	QStringList keywords() const;

	// Serialization of file metadata
	void encode(XdrStream &ws) const;
	void decode(XdrStream &ws);
	QByteArray encode() const;
	static RegInfo decode(const QByteArray &data);

	// Constructors
	inline RegInfo() { }
	inline RegInfo(const RegInfo &other) : at(other.at) { }
	inline RegInfo(const QByteArray &data) { *this = decode(data); }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RegInfo::Attr)

inline XdrStream &operator<<(XdrStream &xs, const RegInfo &fi)
	{ fi.encode(xs); return xs; }
inline XdrStream &operator>>(XdrStream &xs, RegInfo &fi)
	{ fi.decode(xs); return xs; }


} // namespace SST

#endif	// SST_REG_H
