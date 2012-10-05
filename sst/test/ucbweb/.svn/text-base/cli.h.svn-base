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

class QIODevice;


namespace SST {

class Host;
class TestClient;


typedef int Request;			// just the size of the response

struct Page {
	quint32 reqtime;		// request timestamp (Unix time_t)
	Request primary;		// primary web page (i.e., HTML)
	QList<Request> secondary;	// page components - images, etc.
};


// A SockClient instance represents one logical connection to the server.
class SockClient : public QObject
{
	Q_OBJECT

private:
	TestClient *tc;
	QIODevice *dev;
	bool conn;
	QQueue<int> reqs;		// Size of each outstanding request
	int remain;			// Bytes remaining in current request
	bool started;			// We've seen the first byte of this req
	Timer rtxtimer;			// Retransmit timer, for UDP only
	int sockno;			// Socket counter

public:
	SockClient(TestClient *tc, const QHostAddress &addr, quint16 port);

	// Submit a request for an object 'len' bytes in size.
	void request(int len);

	// Request a dynamic priority change on this socket,
	// via a substream message
	void reqPriChange(int newpri);

	inline int nreqs() { return reqs.size(); }
	inline bool done() { return reqs.isEmpty(); }

private:
	void sendreq(int len);

private slots:
	void gotConnected();
	void gotReadyRead();
	void udpReadyRead();
	void udpTimeout();
	void gotError();
};


// A TestClient instance represents our whole trace-driven testing process.
class TestClient : public QObject
{
	friend class SockClient;
	Q_OBJECT

private:
	Host			*host;
	QHostAddress		addr;	// Server address
	quint16			port;	// Server port

	QList<SockClient*>	socks;	// Streams currently in use
	int			count;	// Total requests processed
	int			reqno;	// Req # in current batch
	Time			pstart;	// Start timestamp for this page
	Time			bstart;	// Start timestamp for this batch
	int			totsize; // Total size of all reqs on this page
	int			sockcnt; // Total sockets created so far

	// A sorted list of all the unique users in the trace file
	QList<quint32>		users;

	// The "pages" to request, as per trace file
	QHash<quint32, QList<Page> > pages;

	// For logging results
	QFile			logfile;
	QTextStream		logstrm;

	Timer			pritimer;	// XXX for priority test
	int			prievent;


public:
	TestClient(Host *host, const QHostAddress &addr, quint16 port);

private:
	void check();
	void complete();

private slots:
	void priTimeout();
};

} // namespace SST

#endif	// CLI_H
