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
#include <QtDebug>

#include "bdr.h"

using namespace SST;


char BdrStream::zeros[4];

BdrStream::BdrStream()
:	dev(NULL), st(Ok)
{ }

BdrStream::BdrStream(QIODevice *d)
:	dev(d), st(Ok)
{ }

BdrStream::BdrStream(QByteArray *a, QIODevice::OpenMode mode)
:	dev(new QBuffer(a, this)), st(Ok)
{
	dev->open(mode);
}

BdrStream::BdrStream(const QByteArray &a)
:	st(Ok)
{
	QBuffer *buf = new QBuffer(this);
	buf->buffer() = a;
	buf->open(QIODevice::ReadOnly);
	dev = buf;
}

BdrStream::BdrStream(const BdrStream *parent)
:	QObject(parent), st(Ok)
{
	QBuffer *buf = new QBuffer(this);
	buf->open(parent->dev->openMode());
	dev = buf;
}

void BdrStream::setDevice(QIODevice *d)
{
	if (dev != NULL & dev->parent() == this)
		delete dev;
	dev = d;
}

BdrStream &BdrStream::operator<<(qint16 i)
{
	if (i >= 0x40 && i < ...
		...
}

void BdrStream::readBytes(char *&s, int &len)
{
	// Read the length
	*this >> len;
	if (status() != Ok)
		return;
	if (len < 0)
		return setStatus(ReadCorruptData);

	// Read the data itself
	s = new char[len];
	readRawData(s, len);
}

void BdrStream::writeBytes(const char *s, int len)
{
	*this << len;
	writeRawData(s, len);
}

BdrStream &BdrStream::operator<<(const QByteArray &a)
{
	writeBytes(a.data(), a.size());
	return *this;
}

BdrStream &BdrStream::operator>>(QByteArray &a)
{
	// Read the length word
	int len;
	*this >> len;
	if (status() != Ok)
		return *this;
	if (len < 0) {
		setStatus(ReadCorruptData);
		return *this;
	}

	// Read the data itself
	a.resize(len);
	readRawData(a.data(), len);

	return *this;
}

BdrStream &BdrStream::operator<<(const QString &s)
{
	// Determine the number of actual characters in the string,
	// counting each surrogate pair as only one character.
	int len = s.size();
	const QChar *dat = s.constData();
	int actlen = 0;
	for (int i = 0; i < len; i++) {
		unsigned v = dat[i];
		if (v < 0xd800 || v > 0xdbff)		// skip high surrogates
			actlen++;
	}

	// Encode the string.
	*this << actlen;
	quint32 hi = 0;
	for (int i = 0; i < len; i++) {
		quint32 v = dat[i];
		if (v < 0xd800 || v > 0xdfff)		// simple char
			*this << v;
		else if (v <= 0xdbff)			// high surrogate
			hi = (v - 0xd800);
		else					// low surrogate
			*this << (0x10000 + ((hi << 10) | (v - 0xdc00)));
	}
}

BdrStream &BdrStream::operator>>(QString &s)
{
	// Read the character count
	int len;
	*this >> len;
	if (status() != Ok)
		return;
	if (len < 0) {
		setStatus(ReadCorruptData);
		return *this;
	}

	// Clear the string but optimistically reserve space for 'len' QChars.
	// We might need more if the string requires surrogates.
	s.clear();
	s.reserve(len);
	for (int i = 0; i < len; i++) {
		quint32 ch;
		*this >> ch;
		if (status() != Ok)
			return;
		if (ch < 0x10000) {
			s.append(QChar(ch));
		} else if (ch <= 0x10FFFD) {
			ch -= 0x10000;
			s.append(QChar(0xd800 + (ch & 0x3ff)));
			s.append(QChar(0xdc00 + (ch >> 10)));
		} else {
			qDebug() << this << "non-UCS2 character" << ch;
			// XX insert placeholder char of some kind?
		}
	}
	return *this;
}

QString BdrStream::errorString()
{
	switch (st) {
	case Ok:		return tr("no error");
	case ReadPastEnd:	return tr("truncated data stream");
	case ReadCorruptData:	return tr("corrupt data stream");
	case IOError:		return tr("I/O error");	// XXX
	}
	return tr("Unknown Error");
}

