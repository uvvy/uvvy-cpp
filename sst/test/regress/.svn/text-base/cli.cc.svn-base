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

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include <QByteArray>
#include <QTcpSocket>
#include <QFile>
#include <QTime>
#include <QCoreApplication>
#include <QtDebug>

#include "stream.h"
#include "host.h"
#include "util.h"

#include "sim.h"
#include "main.h"
#include "cli.h"
#include "srv.h"

using namespace SST;


////////// SockClient //////////

SockClient::SockClient(Host *host, const Endpoint &srvep, QObject *parent)
:	QObject(parent), host(host), strm(host, this), remain(0)
{
	//connect(&strm, SIGNAL(connected()),
	//	this, SLOT(gotConnected()));
	connect(&strm, SIGNAL(readyRead()),
		this, SLOT(gotReadyRead()));
	connect(&strm, SIGNAL(error(const QString &)),
		this, SLOT(gotError()));
	strm.connectTo(Ident::fromIpAddress(srvep.addr, srvep.port).id(),
			"regress", "basic");
}

#if 0
void SockClient::gotConnected()
{
	//qDebug("SockClient::gotConnected - sending %d requests", reqs.size());
	Q_ASSERT(!conn);
	conn = true;

	// Send any already-queued requests
	Q_ASSERT(dev->isWritable());
	for (int i = 0; i < reqs.size(); i++)
		sendreq(reqs[i]);
}
#endif

void SockClient::request(int reqlen, int replylen, int pri, int flags)
{
	Q_ASSERT(reqlen == REQLEN);	// XXX fix server
	Q_ASSERT(reqlen >= 8);

	// Send the request on the socket
	QByteArray req;
	req.resize(reqlen);	// We don't care about the contents.
	((quint32*)req.data())[0] = htonl(replylen);
	((quint32*)req.data())[1] = htonl(pri);
	((quint32*)req.data())[2] = htonl(flags);

	int act = strm.write(req);
	Q_ASSERT(act == reqlen); (void)act;

	// Tally the amount of response data we're expecting
	remain += replylen;
}

void SockClient::reqPriChange(int newpri)
{
	// Record the time at which we're sending the request
	reqtime = host->currentTime();

	// Create a substream on which to send the priority change request
	Stream *sub = strm.openSubstream();
	Q_ASSERT(sub);

	// Send the priority change request
	qint32 npri = htonl(newpri);
	int act = sub->write((char*)&npri, 4);
	Q_ASSERT(act == 4);

	// Just fire and forget the substream,
	// leaving SST to handle its delivery.
	delete sub;
}

void SockClient::gotReadyRead()
{
	//int bytes = remain;
	int bytes = qMin(strm.bytesAvailable(), (qint64)remain);
	//bytes = qMin(bytes, 1200);
	if (bytes == 0)
		return;
	//qDebug() << "looks like there's" << bytes << "bytes available";

#if 0
	if (!started) {
		// Log the time we see the first byte of this request
		started = true;
		qint64 elapsed = host->currentTime().usecs - starttime.usecs;
		tc->logstrm << qSetFieldWidth(0) << "rqstart"
			<< qSetFieldWidth(12) << remain << elapsed << endl;
	}
#endif

	Q_ASSERT(bytes > 0);
	QByteArray buf;
	buf.resize(bytes);
	int act = strm.read(buf.data(), bytes);
	Q_ASSERT(act >= 0 && act <= bytes);
	if (act == 0)
		return;

	//qDebug() << this << "got" << act << "bytes," << remain << "remain";
	//printf("%lld %d %d\n", host->currentTime().usecs, sockno,
	//	reqs[0]-remain+act);

	remain -= act;
	if (remain == 0) {
#if 0
		// Log the total size and ending time of this request
		tc->totsize += size;
		qint64 elapsed = tc->host->currentTime().usecs
				- tc->bstart.usecs;
		tc->logstrm << qSetFieldWidth(0) << "rqend  "
			<< qSetFieldWidth(12) << size << elapsed << endl;
#endif
		return done();
	}

	// check for more pipelined responses
	gotReadyRead();
}

void SockClient::gotError()
{
	qFatal("Socket error");
}


////////// BasicClient //////////

BasicClient::BasicClient(Host *host, const Endpoint &srvep)
:	host(host), srvep(srvep),
	timer(host), step(0), ndone(0)
{
	connect(&timer, SIGNAL(timeout(bool)), this, SLOT(timeout()));
	timeout();
}

void BasicClient::timeout()
{
	if (step < nsocks) {
		qDebug() << "Starting request" << step+1;
		socks[step] = new SockClient(host, srvep, this);
		socks[step]->request(REQLEN, 100000, 0, FLG_CLOSE);
		connect(socks[step], SIGNAL(done()), this, SLOT(reqDone()));
		timer.start(400*1000);
	}

	step++;
}

void BasicClient::reqDone()
{
	ndone++;
	qDebug() << "Request" << ndone << "of" << nsocks << "done";
	if (ndone == nsocks)
		success = true;
}

void BasicClient::run()
{
	Simulator sim;
	SimLink link;
	SimHost clihost(&sim);
	SimHost srvhost(&sim);
	link.connect(&clihost, cliaddr, &srvhost, srvaddr);
	BasicClient cli(&clihost, Endpoint(srvaddr, NETSTERIA_DEFAULT_PORT));
	TestServer srv(&srvhost, NETSTERIA_DEFAULT_PORT);
	sim.run();
}

