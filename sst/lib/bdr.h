/** @file bdr.h		Compact byte-oriented data encoding/decoding
 */
#ifndef SST_BDR_H
#define SST_BDR_H

#include <QIODevice>

#include "os.h"

class QByteArray;


namespace SST {

/** BDR encoding/decoding stream.
 * This class provides an interface modeled on QDataStream,
 * but uses a more compact byte-oriented data encoding.
 * Probably not quite as fast as BDR, but much more space-efficient
 * for data objects containing many small integers or enums.
 */
class BdrStream : public QObject
{
public:
	enum Status {
		Ok = 0,
		ReadPastEnd,
		ReadCorruptData,
		IntegerOverflow,
		IOError,
	};

private:
	QIODevice *dev;
	Status st;

public:
	/// Create a fresh BdrStream with no QIODevice.
	/// A device must be set with setDevice() before using.
	BdrStream();

	/// Create an BdrStream to encode to/decode from a given QIODevice.
	/// The caller is responsible for ensuring that the QIODevice pointer
	/// remains valid for the lifetime of this BdrStream.
	BdrStream(QIODevice *d);

	/// Create an BdrStream and an internal QBuffer device
	/// to encode to and/or decode from a specified QByteArray.
	BdrStream(QByteArray *a, QIODevice::OpenMode mode);

	/// Create a read-only BdrStream to decode from a given QByteArray.
	BdrStream(const QByteArray &a);

	/// Create a BdrStream to encode/decode a length-delimited substream
	/// of an existing BdrStream.
	BdrStream(const BdrStream *parent);


	////////// I/O Device //////////

	/// Return the current QIODevice that this BdrStream refers to.
	inline QIODevice *device() { return dev; }

	/// Change this BdrStream to encode to/decode from a different device.
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
	/// This method does not perform any BDR decoding.
	inline void readRawData(void *buf, int len) {
		if (dev->read((char*)buf, len) < len)
			setStatus(ReadPastEnd);
	}

	/// Skip over a given number of bytes on the underlying QIODevice.
	/// This method does not perform any BDR decoding,
	/// and it requires that the underlying device support seeking.
	inline void skipRawData(int len) {
		if (!dev->seek(dev->pos() + len))
			setStatus(IOError);
	}

	/// Write raw data to the underlying QIODevice.
	/// This method does not perform any BDR encoding.
	inline void writeRawData(const void *buf, int len) {
		if (dev->write((const char*)buf, len) < len)
			setStatus(IOError);
	}


	////////// BDR Encoding for High-level Types //////////

	/// BDR-decode an 'opaque' field.
	/// Allocates a new char[] array to hold the opaque data,
	/// and returns the pointer and array length in "out" parameters.
	/// @param s	a variable in which this method returns
	///		a pointer to the opaque data buffer.
	/// @param len	a variable in which this method returns
	///		the length of the opaque data.
	void readBytes(char *&s, int &len);

	/// BDR-encode an 'opaque' field.
	/// @param s	a pointer to the opaque data to encode.
	/// @param len	the length of the opaque data in bytes.
	void writeBytes(const char *s, int len);

	/// BDR-encode a boolean.
	inline BdrStream &operator<<(bool b) {
		if (!dev->putChar(b))
			setStatus(IOError);
		return *this;
	}

	/// Encode an 8-bit integer.
	inline BdrStream &operator<<(qint8 i) {
		if (!dev->putChar(i))
			setStatus(IOError);
		return *this;
	}
	inline BdrStream &operator<<(quint8 i) {
		if (!dev->putChar(i))
			setStatus(IOError);
		return *this;
	}

	/// Encode larger integers.
	BdrStream &operator<<(int i);
	BdrStream &operator<<(qint16 i);
	BdrStream &operator<<(qint32 i);
	BdrStream &operator<<(qint64 i);

	BdrStream &operator<<(unsigned i);
	BdrStream &operator<<(quint16 i);
	BdrStream &operator<<(quint32 i);
	BdrStream &operator<<(quint64 i);

#if 0
	/// Encode a 32-bit IEEE floating-point value (BDR 'float').
	inline BdrStream &operator<<(float f) {
		Q_ASSERT(sizeof(f) == 4);
		return *this << *(qint32*)&f;
	}

	/// Encode a 64-bit IEEE floating-point value (BDR 'double').
	inline BdrStream &operator<<(double f) {
		Q_ASSERT(sizeof(f) == 8);
		return *this << *(qint64*)&f;
	}
#endif


	////////// BDR Decoding for High-level Types //////////

	/// BDR-decode an 8-bit C 'char'
	inline BdrStream &operator>>(char &c) {
		if (!dev->getChar(&c))
			setStatus(IOError);
		return *this;
	}

	// Other simple 8-bit types
	inline BdrStream &operator>>(bool &b) {
		char c; *this >> c; b = (c != 0); return *this; }
	inline BdrStream &operator>>(qint8 &i) {
		char c; *this >> c; i = c; return *this; }
	inline BdrStream &operator>>(quint8 &i) {
		char c; *this >> c; i = c; return *this; }

	// Decode larger integers.
	BdrStream &operator>>(qint16 &i);
	BdrStream &operator>>(qint32 &i);
	BdrStream &operator>>(qint64 &i);

	BdrStream &operator>>(quint16 &i);
	BdrStream &operator>>(quint32 &i);
	BdrStream &operator>>(quint64 &i);


#if 0
	/// Decode a 32-bit IEEE floating-point value (BDR 'float').
	inline BdrStream &operator>>(float &f) {
		Q_ASSERT(sizeof(f) == 4);
		return *this >> *(qint32*)&f;
	}

	/// Decode a 64-bit IEEE floating-point value (BDR 'double').
	inline BdrStream &operator>>(double &f) {
		Q_ASSERT(sizeof(f) == 8);
		return *this >> *(qint64*)&f;
	}
#endif

	/// Encode a QByteArray as a variable-length BDR 'opaque' field
	BdrStream &operator>>(QByteArray &a);

	/// Decode a variable-length BDR 'opaque' field into a QByteArray.
	BdrStream &operator<<(const QByteArray &a);

	/// Encode a QString as an BDR string
	BdrStream &operator<<(const QString &s);

	/// Decode an BDR string into a QString
	BdrStream &operator>>(QString &s);


	/// Return a string describing the last encoding or decoding error
	/// that occurred, if any.
	QString errorString();
};


#if 0
// Helper functions for encoding/decoding vectors and arrays
template<class T>
void xdrEncodeVector(BdrStream &xs, const QList<T> &l, int len)
{
	Q_ASSERT(l.size() == len);
	for (int i = 0; i < len; i++) {
		if (xs.status() != xs.Ok)
			return;
		xs << l.at(i);
	}
}

template<class T>
void xdrDecodeVector(BdrStream &xs, QList<T> &l, int len)
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
void xdrEncodeArray(BdrStream &xs, const QList<T> &l, quint32 maxlen)
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
void xdrDecodeArray(BdrStream &xs, QList<T> &l, quint32 maxlen)
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
inline void xdrEncodeVector(BdrStream &xs, const QByteArray &v, int len)
{
	Q_ASSERT(v.size() == len);
	xs.writeRawData(v.constData(), len);
}

inline void xdrDecodeVector(BdrStream &xs, QByteArray &v, int len)
{
	v.resize(len);
	xs.readRawData(v.data(), len);
}

inline void xdrEncodeArray(BdrStream &xs, const QByteArray &v, quint32 maxlen)
{
	Q_ASSERT((quint32)v.size() <= maxlen);
	(void)maxlen;	// shut up g++ about unused maxlen
	xs << v;
}

inline void xdrDecodeArray(BdrStream &xs, QByteArray &v, quint32 maxlen)
{
	xs >> v;
	if ((quint32)v.size() > maxlen)
		return xs.setStatus(xs.ReadCorruptData);
}



/** Template class representing an BDR "pointer" field.
 * A "pointer" in BDR is semantically not so much a pointer
 * as merely an optional instance of some other type,
 * which can be a forward reference to a type not declared yet.
 * In line with this usage, we map BDR pointers to an BdrPointer class
 * that represents a "captured", automatically-managed instance
 * of the appropriate target type.
 */
template<class T> class BdrPointer
{
private:
	T *i;

public:
	inline BdrPointer() : i(0) { }
	inline BdrPointer(const T &o) : i(new T(o)) { }
	inline BdrPointer(const BdrPointer<T> &o)
		: i(o.i ? new T(*o.i) : 0) { }

	inline ~BdrPointer() { if (i) delete i; }

	inline bool isNull() const { return i == 0; }
	inline void clear() { if (i) { delete i; i = 0; } }
	inline T& alloc() { if (!i) i = new T; return *i; }

	inline BdrPointer<T> &operator=(const BdrPointer<T> &o)
		{ clear(); if (o.i) i = new T(*o.i); }
	inline BdrPointer<T> &operator=(T *o)
		{ clear(); if (o) i = new T(o); }

	inline T* operator->() const { return i; }
	inline T& operator*() const { return *i; }
	inline operator T*() const { return i; }
};

template<class T>
inline BdrStream &operator<<(BdrStream &xs, const BdrPointer<T> &p)
	{ if (p) { xs << true << *p; } else { xs << false; }; return xs; }
template<class T>
inline BdrStream &operator>>(BdrStream &xs, BdrPointer<T> &p)
	{ bool f; xs >> f; if (f) { xs >> p.alloc(); } else p.clear();
	  return xs; }


/** Template class representing a length-delimited BDR option.
 * An "option" is our own extension to BDR, similar to BdrPointer
 * in that it represents an optional instance of some other type,
 * but uses a length-delimited encoding rather than just a boolean flag,
 * so that on decode we can skip over a value we can't understand.
 * For encoding purposes an "option" is equivalent to an opaque<>;
 * the purpose of this template class is merely to make it easier
 * to get at the semantic content _within_ the "opaque" field.
 */
template<class T> class BdrOption
{
private:
	T *i;				// Decoded option value
	QByteArray enc;			// Encoded option value

public:
	inline BdrOption() : i(NULL) { }
	inline BdrOption(const T &o) : i(new T(o)) { }
	inline BdrOption(const BdrOption<T> &o)
		: i(o.i ? new T(*o.i) : NULL), enc(o.enc) { }

	inline ~BdrOption() { if (i) { delete i; i = NULL; } }

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

	inline BdrOption<T> &operator=(const BdrOption<T> &o)
		{ clear(); if (o.i) i = new T(*o.i); enc = o.enc;
		  return *this; }
	inline BdrOption<T> &operator=(T *o)
		{ clear(); if (o) i = new T(o); return *this; }

	inline T* operator->() const { return i; }
	inline T& operator*() const { return *i; }
	inline operator T*() const { return i; }
};

template<class T>
BdrStream &operator<<(BdrStream &xs, const BdrOption<T> &o)
{
	if (!o.isNull()) {
		QByteArray buf;
		BdrStream bxs(&buf, QIODevice::WriteOnly);
		bxs << *o;
		xs << buf;
	} else
		xs << o.getEncoded();
	return xs;
}

template<class T>
BdrStream &operator>>(BdrStream &xs, BdrOption<T> &o)
{
	QByteArray buf;
	xs >> buf;
	if (buf.isEmpty()) {
		o.clear();
		return xs;
	}
	BdrStream bxs(&buf, QIODevice::ReadOnly);
	bxs >> o.alloc();
	if (bxs.status() != bxs.Ok)
		o.setEncoded(buf);
	return xs;
}
#endif

} // namespace SST

#endif	// SST_BDR_H

