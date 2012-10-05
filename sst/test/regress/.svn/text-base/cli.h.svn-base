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
#ifndef CLI_H
#define CLI_H

#include <QHash>
#include <QQueue>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QHostAddress>

#include "timer.h"
#include "stream.h"

class QIODevice;


namespace SST {

class Host;
class TestClient;


// Flag: ask the server to close the stream immediately on sending the reply.
#define FLG_CLOSE	0x01


// A SockClient instance represents one logical connection to the server.
class SockClient : public QObject
{
	Q_OBJECT

private:
	Host *host;
	Stream strm;
	int remain;

	bool started;
	Time reqtime;
	Time starttime;

public:
	SockClient(Host *h, const Endpoint &srvep, QObject *parent = NULL);

	// Submit a request for an object 'len' bytes in size.
	void request(int reqlen, int replylen, int pri, int flags);

	// Request a dynamic priority change on this socket,
	// via a substream message
	void reqPriChange(int newpri);

signals:
	void done();

private slots:
	//void gotConnected();
	void gotReadyRead();
	void gotError();
};


class BasicClient : public QObject
{
	Q_OBJECT

private:
	static const int nsocks = 10;

	Host		*host;
	Endpoint	srvep;
	Timer		timer;
	int		step;
	int		ndone;
	SockClient	*socks[nsocks];

public:
	BasicClient(Host *host, const Endpoint &srvep);

	static void run();

private slots:
	void timeout();
	void reqDone();
};

} // namespace SST

#endif	// CLI_H
