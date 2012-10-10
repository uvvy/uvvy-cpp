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


#include <QtDebug>

#include "reg.h"
#include "xdr.h"
#include "util.h"

using namespace SST;


////////// RegInfo //////////

QList<Endpoint> RegInfo::endpoints() const
{
	XdrStream rs(attr(Endpoints));
	qint32 neps;
	rs >> neps;
	QList<Endpoint> eps;
	if (neps <= 0 || rs.status() != rs.Ok)
		return eps;
	for (int i = 0; i < neps; i++) {
		Endpoint ep;
		rs >> ep;
		eps.append(ep);
	}
	return eps;
}

void RegInfo::setEndpoints(const QList<Endpoint> &eps)
{
	QByteArray buf;
	XdrStream ws(&buf, QIODevice::WriteOnly);
	qint32 neps = eps.size();
	ws << neps;
	for (int i = 0; i < neps; i++)
		ws << eps[i];
	setAttr(Endpoints, buf);
}

QStringList RegInfo::keywords() const
{
	QStringList keywords;
	QHash<Attr, QByteArray>::const_iterator i = at.constBegin();
	while (i != at.constEnd()) {
		if (i.key() & SearchFlag) {
			// Break this string into keywords and add it to our list
			QString str = QString::fromUtf8(i.value());
			keywords += str.split(QRegExp("\\W+"),
						QString::SkipEmptyParts);
		}
		++i;
	}
	return keywords;
}

void RegInfo::encode(XdrStream &ws) const
{
	ws << (qint32)at.size();
	//qDebug() << "encode nattrs" << at.size();
	QHash<Attr, QByteArray>::const_iterator i = at.constBegin();
	while (i != at.constEnd()) {
		ws << (qint32)i.key() << i.value();
		++i;
	}
}

void RegInfo::decode(XdrStream &rs)
{
	// Produce an empty RegInfo if something goes wrong
	at.clear();

	// Decode the attribute count
	qint32 nattrs;
	rs >> nattrs;
	if (rs.status() != rs.Ok || nattrs < 0)
		return;

	// Read the attributes
	//qDebug() << "decode nattrs" << nattrs;
	for (int i = 0; i < nattrs; i++) {
		qint32 tag;
		QByteArray attr;
		rs >> tag >> attr;
		if (rs.status() != rs.Ok) {
			qDebug() << "RegInfo: error decoding attr" << i;
			return;
		}
		at.insert((Attr)tag, attr);
		//qDebug() << "attr" << i << tag << attr;
	}
}

QByteArray RegInfo::encode() const
{
	QByteArray data;
	XdrStream ws(&data, QIODevice::WriteOnly);
	ws << *this;
	return data;
}

RegInfo RegInfo::decode(const QByteArray &data)
{
	XdrStream rs(data);
	RegInfo fi;
	rs >> fi;
	return fi;
}

