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
#include <QBuffer>

#include "xdr.h"

using namespace SST;


char XdrStream::zeros[4];

XdrStream::XdrStream()
:	dev(NULL), st(Ok)
{ }

XdrStream::XdrStream(QIODevice *d)
:	dev(d), st(Ok)
{ }

XdrStream::XdrStream(QByteArray *a, QIODevice::OpenMode mode)
:	dev(new QBuffer(a, this)), st(Ok)
{
	dev->open(mode);
}

XdrStream::XdrStream(const QByteArray &a)
:	st(Ok)
{
	QBuffer *buf = new QBuffer(this);
	buf->buffer() = a;
	buf->open(QIODevice::ReadOnly);
	dev = buf;
}

void XdrStream::setDevice(QIODevice *d)
{
	if (dev != NULL & dev->parent() == this)
		delete dev;
	dev = d;
}

void XdrStream::readPadData(void *buf, int len)
{
	readRawData(buf, len);
	if (len & 3)
		skipRawData(4 - (len & 3));
}

void XdrStream::skipPadData(int len)
{
	skipRawData((len + 3) & ~3);
}

void XdrStream::writePadData(const void *buf, int len)
{
	writeRawData(buf, len);
	if (len & 3)
		writeRawData(zeros, 4 - (len & 3));
}

void XdrStream::readBytes(char *&s, qint32 &len)
{
	// Read the length word
	*this >> len;
	if (status() != Ok)
		return;
	if (len < 0)
		return setStatus(ReadCorruptData);

	// Read the data itself
	s = new char[len];
	readPadData(s, len);
}

void XdrStream::writeBytes(const char *s, qint32 len)
{
	*this << (qint32)len;
	writePadData(s, len);
}

XdrStream &XdrStream::operator<<(const QByteArray &a)
{
	qint32 len = a.size();
	*this << len;
	writePadData(a.data(), len);
	return *this;
}

XdrStream &XdrStream::operator>>(QByteArray &a)
{
	// Read the length word
	qint32 len;
	*this >> len;
	if (status() != Ok)
		return *this;
	if (len < 0) {
		setStatus(ReadCorruptData);
		return *this;
	}

	// Read the data itself
	a.resize(len);
	Q_ASSERT(a.size() == len);
	readPadData(a.data(), len);

	return *this;
}

XdrStream &XdrStream::operator<<(const QString &s)
{
	return *this << s.toUtf8();
}

XdrStream &XdrStream::operator>>(QString &s)
{
	QByteArray a;
	*this >> a;
	s = s.fromUtf8(a.data(), a.size());
	return *this;
}

QString XdrStream::errorString()
{
	switch (st) {
	case Ok:		return tr("no error");
	case ReadPastEnd:	return tr("truncated data stream");
	case ReadCorruptData:	return tr("corrupt data stream");
	case IOError:		return tr("I/O error");	// XXX
	}
	return tr("Unknown Error");
}

