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
#ifndef SST_UTIL_H
#define SST_UTIL_H

#include <QList>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QPointer>
#include <QHostAddress>
#include <QDebug>

#include <openssl/bn.h>


namespace SST {

class XdrStream;


// Sorted lists with incremental insertion via binary search.
template<typename T> class SortList : public QList<T>
{
	// Find the position at which an item is or should be located.
	// May optionally restrict the search to a specified range of items,
	// if something about the location of the item is already known.
	inline int find(const T &value, int lo = 0,
			int hi = QList<T>::size()) const {
		while (lo < hi) {
			int mid = (hi + lo) / 2;
			if (this->at(mid) < value)
				lo = mid+1;
			else
				hi = mid;
		}
		return hi;
	}

	// Find an item via binary search, returning -1 if not present.
	inline int indexOf(const T &value, int from = 0) const {
		int i = pos(value, from);
		if (i >= QList<T>::size() || value < QList<T>::at(i))
			return -1;
		return i;
	}

	// Insert an item at its proper position in the list,
	// before any existing items having the same comparative position.
	inline void insert(const T &value) {
		insert(pos(value), value);
	}
};



// This class holds a set of QPointers,
// whose members automatically disappear as their targets get deleted.
template<typename T> class QPointerSet
{
	// G++ doesn't seem to like the hairy nested template invocations
	// needed to make the internal hash object depend properly on T,
	// so we just make it use QObject and cast away...
	typedef QHash<QObject*, QPointer<QObject> > Hash;
	typedef QHash<QObject*, QPointer<QObject> >::iterator
			HashIter;
	typedef QHash<QObject*, QPointer<QObject> >::const_iterator
			HashConstIter;

	Hash hash;

public:
	inline void insert(T *ptr) { hash.insert(ptr, ptr); }
	inline void remove(T *ptr) { hash.remove(ptr); }
	inline bool contains(T *ptr) { return hash.value(ptr) == ptr; }

	class const_iterator
	{
		HashConstIter i;	// Current position
		HashConstIter e;	// End position

	public:

		// Skip null pointers, moving forward
		inline void skipfwd()
			{ while (i != e && i.value() == NULL) i++; }

		inline const_iterator(const HashConstIter &i,
					const HashConstIter &e)
			: i(i), e(e) { skipfwd(); }

		inline const_iterator &operator++()
			{ i++; skipfwd(); return *this; }

		inline bool operator==(const const_iterator &o) const
			{ return i == o.i; }
		inline bool operator!=(const const_iterator &o) const
			{ return i != o.i; }

		inline T *operator*() const
			{ return reinterpret_cast<T*>(&(*i.value())); }
	};

	inline const_iterator begin() const
		{ return const_iterator(hash.constBegin(), hash.constEnd()); }
	inline const_iterator end() const
		{ return const_iterator(hash.constEnd(), hash.constEnd()); }

	inline const_iterator constBegin() const
		{ return const_iterator(hash.constBegin(), hash.constEnd()); }
	inline const_iterator constEnd() const
		{ return const_iterator(hash.constEnd(), hash.constEnd()); }
};


// Generate cryptographically random and pseudo-random bytes
QByteArray randBytes(int size);
QByteArray pseudoRandBytes(int size);

// Convert between OpenSSL BIGNUMs and QByteArrays
BIGNUM *ba2bn(const QByteArray &ba, BIGNUM *ret = NULL);
QByteArray bn2ba(const BIGNUM *bn);

XdrStream &operator<<(XdrStream &xs, BIGNUM *bn);
XdrStream &operator>>(XdrStream &xs, BIGNUM *&bn);


// Convenient representation for a network endpoint: an (address, port) pair.
struct Endpoint
{
	// Type codes for XDR externalization
	enum Type {
		IPv4 = 1,
		IPv6 = 2
	};

	QHostAddress addr;
	quint16 port;

	Endpoint();
	Endpoint(quint32 ip4addr, quint16 port);
	Endpoint(const QHostAddress &addr, quint16 port);

	inline bool isNull() const { return addr.isNull() && port == 0; }

	inline bool operator==(const Endpoint &other) const
		{ return addr == other.addr && port == other.port; }
	inline bool operator!=(const Endpoint &other) const
		{ return !(addr == other.addr && port == other.port); }

	QString toString() const;

	void encode(XdrStream &xs) const;
	void decode(XdrStream &xs);
};

inline XdrStream &operator<<(XdrStream &xs, const Endpoint &ep)
	{ ep.encode(xs); return xs; }
inline XdrStream &operator>>(XdrStream &xs, Endpoint &ep)
	{ ep.decode(xs); return xs; }

inline QDebug &operator<<(QDebug &ts, const Endpoint &ep)
	{ return ts << ep.addr.toString() << ':' << ep.port; }


// Convert a byte count to a string using standard notation: KB, MB, etc.
QString bytesNumber(qint64 size);


} // namespace SST


// Hash function for QHostAddress (provided by Qt 4.2.x but not 4.1.x)
uint qHash(const QHostAddress &addr);

// Hash function for SST Endpoint structs
uint qHash(const SST::Endpoint &ep);


#endif	// SST_UTIL_H
