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
/** @file xdr.h		Qt-oriented implementation of XDR encoding/decoding.
 */
#ifndef SST_XDR_H
#define SST_XDR_H

#include <QIODevice>

#include "os.h"

class QByteArray;


namespace SST {

/** XDR encoding/decoding stream.
 * This class provides an interface modeled on QDataStream,
 * but uses the XDR encoding as specified in RFC 1832.
 */
class XdrStream : public QObject
{
public:
	enum Status {
		Ok = 0,
		ReadPastEnd,
		ReadCorruptData,
		IOError,
	};

private:
	QIODevice *dev;
	Status st;

	static char zeros[4];

public:
	/// Create a fresh XdrStream with no QIODevice.
	/// A device must be set with setDevice() before using.
	XdrStream();

	/// Create an XdrStream to encode to/decode from a given QIODevice.
	/// The caller is responsible for ensuring that the QIODevice pointer
	/// remains valid for the lifetime of this XdrStream.
	XdrStream(QIODevice *d);

	/// Create an XdrStream and an internal QBuffer device
	/// to encode to and/or decode from a specified QByteArray.
	XdrStream(QByteArray *a, QIODevice::OpenMode mode);

	/// Create a read-only XdrStream to decode from a given QByteArray.
	XdrStream(const QByteArray &a);


	////////// I/O Device //////////

	/// Return the current QIODevice that this XdrStream refers to.
	inline QIODevice *device() { return dev; }

	/// Change this XdrStream to encode to/decode from a different device.
	void setDevice(QIODevice *d);


	////////// Encode/Decode Status //////////

	/// Returns the current encoding/decoding status.
	inline Status status() const { return st; }

	/// Set the current encode/decode status flag.
	inline void setStatus(Status st) { this->st = st; }

	/// Reset the status to the initial Ok state.
	inline void resetStatus() { setStatus(Ok); }


	////////// Raw Read/Write Operations //////////

	/// Returns true if we have read to the end of the underlying device.
	inline bool atEnd() { return dev->atEnd(); }

	/// Read raw data from the underlying QIODevice.
	/// This method does not perform any XDR decoding.
	inline void readRawData(void *buf, int len) {
		if (dev->read((char*)buf, len) < len)
			setStatus(ReadPastEnd);
	}

	/// Skip over a given number of bytes on the underlying QIODevice.
	/// This method does not perform any XDR decoding,
	/// and it requires that the underlying device support seeking.
	inline void skipRawData(int len) {
		if (!dev->seek(dev->pos() + len))
			setStatus(IOError);
	}

	/// Write raw data to the underlying QIODevice.
	/// This method does not perform any XDR encoding.
	inline void writeRawData(const void *buf, int len) {
		if (dev->write((const char*)buf, len) < len)
			setStatus(IOError);
	}


	////////// Padded Read/Write //////////

	/// Read a given number of bytes of raw data,
	/// then skip 0-3 bytes to pad the raw block length
	/// to a 4-byte boundary as the XDR spec requires.
	void readPadData(void *buf, int len);

	/// Skip @a len bytes, rounded up to the next 4-byte boundary.
	void skipPadData(int len);

	/// Write a specified block of raw data,
	/// followed by 0-3 zero bytes to pad the data block
	/// to a 4-byte boundary as the XDR spec requires.
	void writePadData(const void *buf, int len);


	////////// XDR Encoding for High-level Types //////////

	/// XDR-decode an 'opaque' field.
	/// Allocates a new char[] array to hold the opaque data,
	/// and returns the pointer and array length in "out" parameters.
	/// @param s	a variable in which this method returns
	///		a pointer to the opaque data buffer.
	/// @param len	a variable in which this method returns
	///		the length of the opaque data.
	void readBytes(char *&s, qint32 &len);

	/// XDR-encode an 'opaque' field.
	/// @param s	a pointer to the opaque data to encode.
	/// @param len	the length of the opaque data in bytes.
	void writeBytes(const char *s, qint32 len);

	/// XDR-encode a boolean.
	inline XdrStream &operator<<(bool b) {
		qint32 v = htonl(b);
		writeRawData(&v, 4);
		return *this;
	}

	/// Encode an 8-bit integer (XDR 'char').
	inline XdrStream &operator<<(qint8 i) {
		qint32 v = htonl(i);
		writeRawData(&v, 4);
		return *this;
	}

	/// Encode a 16-bit integer (XDR 'short').
	inline XdrStream &operator<<(qint16 i) {
		qint32 v = htonl(i);
		writeRawData(&v, 4);
		return *this;
	}

	/// Encode a 32-bit integer (XDR 'int').
	inline XdrStream &operator<<(qint32 i) {
		qint32 v = htonl(i);
		writeRawData(&v, 4);
		return *this;
	}

	/// Encode a 64-bit integer (XDR 'hyper').
	inline XdrStream &operator<<(qint64 i) {
		qint32 v[2] = { htonl(i >> 32), htonl(i) };
		Q_ASSERT(sizeof(v) == 8);
		writeRawData(&v, 8);
		return *this;
	}

	/// Encode an 8-bit unsigned integer (XDR 'unsigned char').
	inline XdrStream &operator<<(quint8 i) {
		return *this << (qint8)i;
	}

	/// Encode a 16-bit unsigned integer (XDR 'unsigned short').
	inline XdrStream &operator<<(quint16 i) {
		return *this << (qint16)i;
	}

	/// Encode a 32-bit unsigned integer (XDR 'unsigned int').
	inline XdrStream &operator<<(quint32 i) {
		return *this << (qint32)i;
	}

	/// Encode a 64-bit unsigned integer (XDR 'unsigned hyper').
	inline XdrStream &operator<<(quint64 i) {
		return *this << (qint64)i;
	}

	/// Encode a 32-bit IEEE floating-point value (XDR 'float').
	inline XdrStream &operator<<(float f) {
		Q_ASSERT(sizeof(f) == 4);
		return *this << *(qint32*)&f;
	}

	/// Encode a 64-bit IEEE floating-point value (XDR 'double').
	inline XdrStream &operator<<(double f) {
		Q_ASSERT(sizeof(f) == 8);
		return *this << *(qint64*)&f;
	}


	////////// XDR Decoding for High-level Types //////////

	/// XDR-decode a boolean.
	inline XdrStream &operator>>(bool &b) {
		qint32 v;
		readRawData(&v, 4);
		b = (v != 0);
		return *this;
	}

	/// Decode an 8-bit integer (XDR 'char').
	inline XdrStream &operator>>(qint8 &i) {
		qint32 v;
		readRawData(&v, 4);
		i = ntohl(v);
		return *this;
	}

	/// Decode a 16-bit integer (XDR 'short').
	inline XdrStream &operator>>(qint16 &i) {
		qint32 v;
		readRawData(&v, 4);
		i = ntohl(v);
		return *this;
	}

	/// Decode a 32-bit integer (XDR 'int').
	inline XdrStream &operator>>(qint32 &i) {
		qint32 v;
		readRawData(&v, 4);
		i = ntohl(v);
		return *this;
	}

	/// Decode a 64-bit integer (XDR 'hyper').
	inline XdrStream &operator>>(qint64 &i) {
		quint32 v[2];
		Q_ASSERT(sizeof(v) == 8);
		readRawData(&v, 8);
		i = ((quint64)(quint32)ntohl(v[0]) << 32) |
			(quint32)ntohl(v[1]);
		return *this;
	}

	/// Decode an 8-bit unsigned integer (XDR 'unsigned char').
	inline XdrStream &operator>>(quint8 &i) {
		return *this >> *(qint8*)&i;
	}

	/// Decode a 16-bit unsigned integer (XDR 'unsigned short').
	inline XdrStream &operator>>(quint16 &i) {
		return *this >> *(qint16*)&i;
	}

	/// Decode a 32-bit unsigned integer (XDR 'unsigned int').
	inline XdrStream &operator>>(quint32 &i) {
		return *this >> *(qint32*)&i;
	}

	/// Decode a 64-bit unsigned integer (XDR 'unsigned hyper').
	inline XdrStream &operator>>(quint64 &i) {
		return *this >> *(qint64*)&i;
	}

	/// Decode a 32-bit IEEE floating-point value (XDR 'float').
	inline XdrStream &operator>>(float &f) {
		Q_ASSERT(sizeof(f) == 4);
		return *this >> *(qint32*)&f;
	}

	/// Decode a 64-bit IEEE floating-point value (XDR 'double').
	inline XdrStream &operator>>(double &f) {
		Q_ASSERT(sizeof(f) == 8);
		return *this >> *(qint64*)&f;
	}

	/// Encode a QByteArray as a variable-length XDR 'opaque' field
	XdrStream &operator>>(QByteArray &a);

	/// Decode a variable-length XDR 'opaque' field into a QByteArray.
	XdrStream &operator<<(const QByteArray &a);

	/// Encode a QString as an XDR 'string' using UTF-8 encoding
	XdrStream &operator<<(const QString &s);

	/// Decode an XDR 'string' into a QString using UTF-8 encoding
	XdrStream &operator>>(QString &s);


	/// Return a string describing the last encoding or decoding error
	/// that occurred, if any.
	QString errorString();
};


// Helper functions for encoding/decoding vectors and arrays
template<class T>
void xdrEncodeVector(XdrStream &xs, const QList<T> &l, int len)
{
	Q_ASSERT(l.size() == len);
	for (int i = 0; i < len; i++) {
		if (xs.status() != xs.Ok)
			return;
		xs << l.at(i);
	}
}

template<class T>
void xdrDecodeVector(XdrStream &xs, QList<T> &l, int len)
{
	l.clear();
	for (int i = 0; i < len; i++) {
		if (xs.status() != xs.Ok)
			return;
		T v;
		xs >> v;
		l.append(v);
	}
}

template<class T>
void xdrEncodeArray(XdrStream &xs, const QList<T> &l, quint32 maxlen)
{
	quint32 len = l.size();
	Q_ASSERT(len <= maxlen);
	(void)maxlen;	// shut up g++ about unused maxlen
	xs << len;
	for (quint32 i = 0; i < len; i++) {
		if (xs.status() != xs.Ok)
			return;
		xs << l.at(i);
	}
}

template<class T>
void xdrDecodeArray(XdrStream &xs, QList<T> &l, quint32 maxlen)
{
	quint32 len;
	xs >> len;
	if (len > maxlen)
		return xs.setStatus(xs.ReadCorruptData);
	l.clear();
	for (quint32 i = 0; i < len; i++) {
		if (xs.status() != xs.Ok)
			return;
		T v;
		xs >> v;
		l.append(v);
	}
}


// Helper functions for encoding/decoding byte vectors and arrays
inline void xdrEncodeVector(XdrStream &xs, const QByteArray &v, int len)
{
	Q_ASSERT(v.size() == len);
	xs.writeRawData(v.constData(), len);
}

inline void xdrDecodeVector(XdrStream &xs, QByteArray &v, int len)
{
	v.resize(len);
	xs.readRawData(v.data(), len);
}

inline void xdrEncodeArray(XdrStream &xs, const QByteArray &v, quint32 maxlen)
{
	Q_ASSERT((quint32)v.size() <= maxlen);
	(void)maxlen;	// shut up g++ about unused maxlen
	xs << v;
}

inline void xdrDecodeArray(XdrStream &xs, QByteArray &v, quint32 maxlen)
{
	xs >> v;
	if ((quint32)v.size() > maxlen)
		return xs.setStatus(xs.ReadCorruptData);
}



/** Template class representing an XDR "pointer" field.
 * A "pointer" in XDR is semantically not so much a pointer
 * as merely an optional instance of some other type,
 * which can be a forward reference to a type not declared yet.
 * In line with this usage, we map XDR pointers to an XdrPointer class
 * that represents a "captured", automatically-managed instance
 * of the appropriate target type.
 */
template<class T> class XdrPointer
{
private:
	T *i;

public:
	inline XdrPointer() : i(0) { }
	inline XdrPointer(const T &o) : i(new T(o)) { }
	inline XdrPointer(const XdrPointer<T> &o)
		: i(o.i ? new T(*o.i) : 0) { }

	inline ~XdrPointer() { if (i) delete i; }

	inline bool isNull() const { return i == 0; }
	inline void clear() { if (i) { delete i; i = 0; } }
	inline T& alloc() { if (!i) i = new T; return *i; }

	inline XdrPointer<T> &operator=(const XdrPointer<T> &o)
		{ clear(); if (o.i) i = new T(*o.i); }
	inline XdrPointer<T> &operator=(T *o)
		{ clear(); if (o) i = new T(o); }

	inline T* operator->() const { return i; }
	inline T& operator*() const { return *i; }
	inline operator T*() const { return i; }
};

template<class T>
inline XdrStream &operator<<(XdrStream &xs, const XdrPointer<T> &p)
	{ if (p) { xs << true << *p; } else { xs << false; }; return xs; }
template<class T>
inline XdrStream &operator>>(XdrStream &xs, XdrPointer<T> &p)
	{ bool f; xs >> f; if (f) { xs >> p.alloc(); } else p.clear();
	  return xs; }


/** Template class representing a length-delimited XDR option.
 * An "option" is our own extension to XDR, similar to XdrPointer
 * in that it represents an optional instance of some other type,
 * but uses a length-delimited encoding rather than just a boolean flag,
 * so that on decode we can skip over a value we can't understand.
 * For encoding purposes an "option" is equivalent to an opaque<>;
 * the purpose of this template class is merely to make it easier
 * to get at the semantic content _within_ the "opaque" field.
 */
template<class T> class XdrOption
{
private:
	T *i;				// Decoded option value
	QByteArray enc;			// Encoded option value

public:
	inline XdrOption() : i(NULL) { }
	inline XdrOption(const T &o) : i(new T(o)) { }
	inline XdrOption(const XdrOption<T> &o)
		: i(o.i ? new T(*o.i) : NULL), enc(o.enc) { }

	inline ~XdrOption() { if (i) { delete i; i = NULL; } }

	/// An option is "null" if we don't have it in its decoded form.
	inline bool isNull() const { return i == NULL; }

	/// An option is "empty" if we don't have it in any form.
	inline bool isEmpty() const { return i == NULL && enc.isEmpty(); }

	// Set or clear the item's decoded representation
	inline T& alloc() { enc.clear(); if (!i) i = new T; return *i; }
	inline void clear() { enc.clear(); if (i) { delete i; i = NULL; } }

	// Set or clear the item's encoded representation
	inline QByteArray getEncoded() const { return enc; }
	inline void setEncoded(const QByteArray &e)
		{ clear(); enc = e; }

	inline XdrOption<T> &operator=(const XdrOption<T> &o)
		{ clear(); if (o.i) i = new T(*o.i); enc = o.enc;
		  return *this; }
	inline XdrOption<T> &operator=(T *o)
		{ clear(); if (o) i = new T(o); return *this; }

	inline T* operator->() const { return i; }
	inline T& operator*() const { return *i; }
	inline operator T*() const { return i; }
};

template<class T>
XdrStream &operator<<(XdrStream &xs, const XdrOption<T> &o)
{
	if (!o.isNull()) {
		QByteArray buf;
		XdrStream bxs(&buf, QIODevice::WriteOnly);
		bxs << *o;
		xs << buf;
	} else
		xs << o.getEncoded();
	return xs;
}

template<class T>
XdrStream &operator>>(XdrStream &xs, XdrOption<T> &o)
{
	QByteArray buf;
	xs >> buf;
	if (buf.isEmpty()) {
		o.clear();
		return xs;
	}
	XdrStream bxs(&buf, QIODevice::ReadOnly);
	bxs >> o.alloc();
	if (bxs.status() != bxs.Ok)
		o.setEncoded(buf);
	return xs;
}

} // namespace SST

#endif	// SST_XDR_H
