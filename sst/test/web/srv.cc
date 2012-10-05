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

#include <QFile>
#include <QtDebug>

#include "stream.h"
#include "util.h"

#include "srv.h"

using namespace SST;


////////// WebImage //////////



////////// WebServer //////////

WebServer::WebServer(Host *host, quint16)
:	host(host)
{
	srv = new StreamServer(host, this);
	if (!srv->listen("webtest", "SST prioritized web browser demo",
			"basic", "Basic web request protocol"))
		qFatal("Can't listen on 'webtest' service name");
	connect(srv, SIGNAL(newConnection()),
		this, SLOT(gotConnection()));
}

void WebServer::gotConnection()
{
	Stream *strm = srv->accept();
	if (!strm)
		return;

	strm->setChildReceiveBuffer(sizeof(qint32));	// sizeof pri chng req
	strm->listen(Stream::BufLimit);

	strm->setParent(this);
	connect(strm, SIGNAL(readyReadMessage()),
		this, SLOT(connRead()));
	connect(strm, SIGNAL(readyReadDatagram()),
		this, SLOT(connSub()));
	connect(strm, SIGNAL(reset(const QString &)),
		this, SLOT(connReset()));

	// Check for more queued incoming connections
	gotConnection();
}

void WebServer::connRead()
{
	Stream *strm = (Stream*)sender();
	QByteArray msg = strm->readMessage();
	if (msg.isEmpty())
		return;

	// Interpret the message as a filename to retrieve within the page
	QString name = "page/" + QString::fromAscii(msg);
	QFile f(name);
	if (!f.open(QIODevice::ReadOnly)) {
		bad:
		qDebug() << "Can't open requested object " << name;
		strm->shutdown(strm->Reset);	// Kinda severe, but...
		strm->deleteLater();
		return;
	}

	QByteArray dat = f.readAll();
	if (dat.isEmpty())
		goto bad;

	strm->writeMessage(dat);

	// Go back and read more messages if available
	connRead();
}

void WebServer::connSub()
{
	Stream *strm = (Stream*)sender();

	while (true) {
		qDebug() << "Got priority change request";
		qint32 buf;
		int act = strm->readDatagram((char*)&buf, sizeof(buf));
		if (act <= 0)
			return;
		Q_ASSERT(act == sizeof(buf));

		int newpri = ntohl(buf);
		strm->setPriority(newpri);
	}
}

void WebServer::connReset()
{
	Stream *strm = (Stream*)sender();
	qDebug() << strm << "reset by client";
	strm->deleteLater();
}

