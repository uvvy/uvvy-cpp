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
#include <unistd.h>	// XXX sleep
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

#include "main.h"
#include "cli.h"
#include "tcp.h"

using namespace SST;


// Client behavior parameters
#define PERSIST		false		// allow persistent connections
#define PIPELEN		1		// # pipelined requests per stream
#define MAXCONN		99		// Max # of concurrent connections


#define PRI_TEST	1	// XXX


static bool connasync = true;		// SST: connect asynchronously


// The following stuff based on 'logparse.h' in UCB trace tools.

#define PB_CLNT_NO_CACHE       1
#define PB_CLNT_KEEP_ALIVE     2
#define PB_CLNT_CACHE_CNTRL    4
#define PB_CLNT_IF_MOD_SINCE   8
#define PB_CLNT_UNLESS         16

#define PB_SRVR_NO_CACHE       1
#define PB_SRVR_CACHE_CNTRL    2
#define PB_SRVR_EXPIRES        4
#define PB_SRVR_LAST_MODIFIED  8

struct TraceEntry
{
	quint32 crs;            /* client request seconds */
	quint32 cru;            /* client request microseconds */
	quint32 srs;            /* server first response byte seconds */
	quint32 sru;            /* server first response byte microseconds */
	quint32 sls;            /* server last response byte seconds */
	quint32 slu;            /* server last response byte microseconds */
	quint32 cip;            /* client IP address */
	quint16 cpt;            /* client port */
	quint32 sip;            /* server IP address */
	quint16 spt;            /* server port */
	quint8  cprg;    	/* client headers/pragmas */
	quint8  sprg;    	/* server headers/pragmas */
	/* If a date is FFFFFFFF, it was missing/unreadable/unapplicable in trace */
	quint32 cims;           /* client IF-MODIFIED-SINCE date, if applicable */
	quint32 sexp;           /* server EXPIRES date, if applicable */
	quint32 slmd;           /* server LAST-MODIFIED, if applicable */
	quint32 rhl;            /* response HTTP header length */
	quint32 rdl;            /* response data length, not including header */
	quint16 urllen;         /* url length, not including NULL term */
	QByteArray url;		/* request url, e.g. "GET / HTTP/1.0", + '\0' */
};



////////// SockClient //////////

SockClient::SockClient(TestClient *tc, const QHostAddress &addr, quint16 port)
:	QObject(tc), tc(tc), conn(false),
	rtxtimer(tc->host)
{

	sockno = tc->sockcnt++;

	switch (testproto) {
	case TESTPROTO_SST: {
		Stream *strm = new Stream(tc->host, this);
		if (connasync)
			conn = true;	// Just pretend we're connected already
		else
			connect(strm, SIGNAL(connected()),
				this, SLOT(gotConnected()));
		connect(strm, SIGNAL(readyRead()),
			this, SLOT(gotReadyRead()));
		connect(strm, SIGNAL(error(const QString &)),
			this, SLOT(gotError()));
#ifdef PRI_TEST
		if (sockno == 2)
			strm->setPriority(1);
#endif
		strm->connectTo(Ident::fromIpAddress(addr, port).id(),
				"ucbtest", "ucbtestproto");
		dev = strm;
		break; }
	case TESTPROTO_TCP: {
		TcpStream *strm = new TcpStream(tc->host, this);
		connect(strm, SIGNAL(connected()),
			this, SLOT(gotConnected()));
		connect(strm, SIGNAL(readyRead()),
			this, SLOT(gotReadyRead()));
		connect(strm, SIGNAL(error(const QString &)),
			this, SLOT(gotError()));
		strm->connectToHost(addr, port);
		dev = strm;
		break; }
	case TESTPROTO_NAT: {
		QTcpSocket *tsock = new QTcpSocket(this);
		connect(tsock, SIGNAL(connected()),
			this, SLOT(gotConnected()));
		connect(tsock, SIGNAL(readyRead()),
			this, SLOT(gotReadyRead()));
		connect(tsock, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(gotError()));
		tsock->connectToHost(addr, port);
		dev = tsock;
		break; }
	case TESTPROTO_UDP: {
		qDebug() << "New UDP socket";
		QUdpSocket *udpsock = new QUdpSocket(this);
		connect(udpsock, SIGNAL(readyRead()),
			this, SLOT(udpReadyRead()));
		connect(udpsock, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(gotError()));
		udpsock->connectToHost(addr, port+2);
		dev = udpsock;
		conn = true;
		connect(&rtxtimer, SIGNAL(timeout(bool)),
			this, SLOT(udpTimeout()));
		break; }
	}
}

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

void SockClient::request(int len)
{
#if 0	// XX test generator for TCP/UDP/SST transaction scaling test
static int x = 1000000;
len = x;
if (x == 1000000)
	x = 15;
else {
	x = x*2+1;
	if (x > 1999999) x = 15;
}
#endif

#ifdef PRI_TEST
len = 3*1024*1024/2;
#endif

	// Enqueue a record of the request
	Q_ASSERT(reqs.size() < PIPELEN);
	if (reqs.isEmpty()) {
		remain = len;
		started = false;
	}
	reqs.enqueue(len);

	// Send the request immediately if we're already connected;
	// otherwise gotConnected() will send it later.
	if (conn)
		sendreq(len);

	if (testproto == TESTPROTO_UDP)
		rtxtimer.start();
}

void SockClient::sendreq(int len)
{
	// Send the request on the socket
	QByteArray req(pseudoRandBytes(REQLEN));
	*(quint32*)req.data() = htonl(len);

	int pri = 0;
#ifdef PRI_TEST
	pri = sockno == 2 ? 1 : 0;
#endif
	((quint32*)req.data())[1] = htonl(pri);

	int act = dev->write(req);
	Q_ASSERT(act == REQLEN); (void)act;
}

void SockClient::reqPriChange(int newpri)
{
	Q_ASSERT(testproto == TESTPROTO_SST);
	Stream *strm = (Stream*)dev;

	// Create a substream on which to send the priority change request
	Stream *sub = strm->openSubstream();
	Q_ASSERT(sub);

	qint32 npri = htonl(newpri);
	int act = sub->write((char*)&npri, 4);
	Q_ASSERT(act == 4);
	sub->shutdown(Stream::Write);

	// XXX make it self-destruct when server disconnects
}

void SockClient::gotReadyRead()
{
	if (done())
		return;

	//int bytes = remain;
	int bytes = qMin(dev->bytesAvailable(), (qint64)remain);
	bytes = qMin(bytes, 1200);
	if (bytes == 0)
		return;
	//qDebug() << "looks like there's" << bytes << "bytes available";

	if (!started) {
		// Log the time we see the first byte of this request
		started = true;
		qint64 elapsed = tc->host->currentTime().usecs
				- tc->bstart.usecs;
		tc->logstrm << qSetFieldWidth(0) << "rqstart"
			<< qSetFieldWidth(12) << remain << elapsed << endl;
	}

	Q_ASSERT(bytes > 0);
	QByteArray buf;
	buf.resize(bytes);
	int act = dev->read(buf.data(), bytes);
	Q_ASSERT(act >= 0 && act <= bytes);
	if (act == 0)
		return;

	//qDebug() << this << "got" << act << "bytes:"
	//	<< (reqs[0]-remain+act) << "of" << reqs[0]
	//	<< "nreqs" << nreqs();
	printf("%lld %d %d\n", tc->host->currentTime().usecs, sockno,
		reqs[0]-remain+act);

	remain -= act;
	if (remain == 0) {
		int size = reqs.dequeue();

		// Log the total size and ending time of this request
		tc->totsize += size;
		qint64 elapsed = tc->host->currentTime().usecs
				- tc->bstart.usecs;
		tc->logstrm << qSetFieldWidth(0) << "rqend  "
			<< qSetFieldWidth(12) << size << elapsed << endl;

		if (!reqs.isEmpty()) {
			remain = reqs.head();
			started = false;
		}
		tc->check();
	}

	// check for more pipelined responses
	gotReadyRead();
}

void SockClient::udpReadyRead()
{
	QUdpSocket *usock = (QUdpSocket*)dev;
	if (done() || !usock->hasPendingDatagrams())
		return;

	QByteArray buf;
	buf.resize(remain*4);
	int act = ((QUdpSocket*)dev)->readDatagram(buf.data(), remain*4);
	if (act < 0)
		return;
	Q_ASSERT(act <= remain);
	if (act < remain) {
		qDebug() << "got wrong size datagram:" << act;
		return udpReadyRead();
	}

	qDebug() << "got" << act << "bytes of" << reqs[0] << "nreqs" << nreqs();

	int size = reqs.dequeue();
	qint64 elapsed = tc->host->currentTime().usecs
			- tc->bstart.usecs;
	tc->logstrm << "req " << size << ' ' << elapsed << endl;
	if (!reqs.isEmpty()) {
		remain = reqs.head();
		started = false;
	}

	rtxtimer.stop();

	tc->check();
	return udpReadyRead();
}

void SockClient::udpTimeout()
{
	qDebug() << "UDP timeout";
	Q_ASSERT(nreqs() == 1);

	int len = reqs[0];
	sendreq(len);

	rtxtimer.restart();
}

void SockClient::gotError()
{
	if (testproto == TESTPROTO_UDP)
		qFatal("UDP socket error: %s",
			((QUdpSocket*)dev)->errorString().toAscii().data());
	qFatal("Socket error");
}


////////// TestClient //////////

static bool isSecondary(const QByteArray &ext)
{
	return ext == ".gif" || ext == ".jpg" || ext == ".jpeg"
		|| ext == ".tif" || ext == ".jfif" || ext == ".bmp"
		|| ext == ".pjpg" || ext == "tgif" || ext == ".xbm"
		|| ext == ".anm"
		|| ext == ".mid" || ext == ".aif" || ext == ".aiff"
		|| ext == ".au" || ext == ".vox" || ext == ".wav"
		|| ext == ".class" || ext == ".jar";
}

TestClient::TestClient(Host *host, const QHostAddress &addr, quint16 port)
:	host(host), addr(addr), port(port),
	count(0), reqno(0), sockcnt(0),
	logfile("log"), logstrm(&logfile),
	pritimer(host)
{
	QFile file("trace");
	if (!file.open(QIODevice::ReadOnly))
		qFatal("Can't open trace file");

	// Read in the whole trace file,
	// sorting entries into one bucket per client IP address
	qDebug() << "Reading trace file...";
	QHash<quint32, TraceEntry> curent;
	QHash<quint32, Page> curpage;
	QHash<QByteArray, int> extcount;
	QDataStream rds(&file);
	int totreqs = 0;
	double mindelay = 1000.0, cumdelay = 0;
	double maxbps = 0, cumbps = 0;
	int cumdelayreqs = 0, cumbpsreqs = 0;
	while (!file.atEnd()) {

		// Read an entry
		TraceEntry te;
		rds >> te.crs >> te.cru >> te.srs >> te.sru >> te.sls >> te.slu
			>> te.cip >> te.cpt >> te.sip >> te.spt
			>> te.cprg >> te.sprg
			>> te.cims >> te.sexp >> te.slmd >> te.rhl >> te.rdl
			>> te.urllen;
		te.url.resize(te.urllen);
		rds.readRawData(te.url.data(), te.urllen);
		if (rds.status() != rds.Ok) {
			if (rds.status() == rds.ReadPastEnd) {
				Q_ASSERT(file.atEnd());
				break;
			}
			qFatal("Error decoding trace file");
		}

		// Get total reply length;
		// ignore records with null replies.
		int replen = te.rhl + te.rdl;
		if (replen == 0)
			continue;

		// Break apart the URL string
		int methodspc = te.url.indexOf(' ');
		Q_ASSERT(methodspc > 0);
		QByteArray method = te.url.mid(0, methodspc);
		if (method != "GET" && method != "POST" && method != "HEAD")
			qWarning("Unknown HTTP method %s", method.data());

		int urlspc = te.url.indexOf(' ', methodspc+1);
		QByteArray url, httpvers;
		if (urlspc >= 0) {
			url = te.url.mid(methodspc+1, urlspc-(methodspc+1));
			httpvers = te.url.mid(urlspc+1);
		} else
			url = te.url.mid(methodspc+1);
		if (!httpvers.isEmpty() && httpvers != "HTTP/1.0")
			qWarning("Unknown HTTP version %s", httpvers.data());

		// Extract the extension from the URL
		int extidx = url.indexOf('.');
		Q_ASSERT(extidx >= 0);
		extidx++;
		while (extidx < url.size() && 
				(url[extidx] == 'q' || url[extidx] == 'c'))
			extidx++;	// skip flag chars
		Q_ASSERT(extidx == url.size() || url[extidx] == '.');
		QByteArray ext = url.mid(extidx).toLower();

		// Keep a tally of all the unique extensions we see
		extcount[ext]++;
		totreqs++;

		// Also accumulate some timing statistics from the traces
		if (te.crs != 0xffffffff && te.cru != 0xffffffff &&
				te.srs != 0xffffffff && te.sru != 0xffffffff &&
				te.sls != 0xffffffff && te.slu != 0xffffffff) {
			quint64 cr = (quint64)(te.crs * 1000000) + te.cru;
			quint64 sr = (quint64)(te.srs * 1000000) + te.sru;
			quint64 sl = (quint64)(te.sls * 1000000) + te.slu;
			//qDebug() << te.cru << te.crs << te.sru << te.srs
			//	<< te.slu << te.sls;

			// Request-to-response delay in seconds
			double delay = (double)(sr - cr) / 1000000.0;
			Q_ASSERT(delay > 0);
			if (delay < 10.0) {
				// Only include remotely sensible delays
				mindelay = qMin(mindelay, delay);
				cumdelay += delay;
				cumdelayreqs++;
			}

			// Response download bandwidth in bytes per second
			double resptime = (double)(sl - sr) / 1000000.0;
			Q_ASSERT(resptime > 0);
			double bps = (double)replen / resptime;
			if (replen > 10000) {
				// Only use decent-size responses to calc BW
				maxbps = qMax(maxbps, bps);
				cumbps += bps;
				cumbpsreqs++;
			}

			//qDebug() << te.url << "delay" << delay
			//	<< "resptime" << resptime
			//	<< "resplen" << replen
			//	<< "bps" << bps;
		}

		// Find or create the entries for this user's current page
		TraceEntry &ce = curent[te.cip];
		Page &cp = curpage[te.cip];

		// If this looks like a new web page, flush the old one.
		if (!ce.url.isEmpty() &&
			(!isSecondary(ext)		// if not an image
			 || te.sip != ce.sip		// if server changed
			 || (te.crs-ce.crs) > 60)) {	// if 1 min elapsed
			pages[te.cip].append(cp);
			ce.url.clear();
		}

		// Record the request as appropriate
		if (ce.url.isEmpty()) {
			cp.reqtime = te.crs;
			cp.primary = replen;
			cp.secondary.clear();
			//qDebug() << "NEW PAGE:";
		} else {
			cp.secondary.append(replen);
		}
		//qDebug() << te.cip << te.url << ext << replen;

		// Save this entry as the last entry for this user
		ce = te;
	}
	//Q_ASSERT(file.status() == file.Ok);

	// Flush all remaining pages
	foreach (quint32 cip, curent.keys()) {
		if (!curent[cip].url.isEmpty())
			pages[cip].append(curpage[cip]);
	}

	// Build the sorted users list
	users = pages.keys();
	qSort(users);

	file.close();

	// Show extensions
	qDebug() << "URL extensions seen:";
	QList<QByteArray> extlist = extcount.keys();
	qSort(extlist);
	foreach (const QByteArray &ext, extlist)
		if (extcount[ext] > 1)
			qDebug() << " " << extcount[ext] << ext;
	qDebug() << "Total:" << totreqs << "requests";

	// Show original performance statistics
	qDebug() << "Delay: avg" << (cumdelay / cumdelayreqs)
			<< "over" << cumdelayreqs << "min" << mindelay;
	qDebug() << "Bandwidth: avg" << (cumbps / cumbpsreqs)
			<< "over" << cumbpsreqs << "max" << maxbps;

	// Open the log file
	if (!logfile.open(QIODevice::WriteOnly | QIODevice::Append))
		qFatal("can't open log output file");

#ifdef PRI_TEST
	connect(&pritimer, SIGNAL(timeout(bool)),
		this, SLOT(priTimeout()));
	pritimer.start(5*1000*1000);
	prievent = 0;
#endif

	// Start the ball rolling!
	check();
}

void TestClient::check()
{
	// Find the current client's IP address
	if (users.isEmpty())
		return complete();	// all done with all users
	quint32 cip = users.first();

	// The current client's list of pages to download
	QList<Page> &cps = pages[cip];
	if (cps.isEmpty()) {
		users.removeFirst();	// all done with this user
		qDebug() << "User" << cip << "complete,"
			<< users.size() << "more";
		return check();
	}

	// The current page being downloaded
	Page &cp = cps.first();

	// The current batch of requests - one if primary, many if secondary
	QList<Request> batch;
	if (cp.primary) {
		if (reqno == 0) {
			// Starting a new page - record the page start time.
			//qDebug() << "New page";
			pstart = host->currentTime();
			totsize = 0;
		}
		batch.append(cp.primary);
#ifdef PRI_TEST
batch.append(cp.primary);	// XXX
#endif
	} else
		batch = cp.secondary;

	if (reqno == 0) {
		// Starting a new batch - record the batch start time.
		//sleep(1);
		//qDebug() << "New batch: size" << batch.size();
		bstart = host->currentTime();
	}

	bool batchdone;
	if (!PERSIST) {
		// Using non-persistent connections.
		// First garbage collect any that have completed.
		foreach (SockClient *sock, socks)
			if (sock->done()) {
				socks.removeAll(sock);
				sock->deleteLater();
			}

		// Launch any new connections allowed/needed
		while (socks.size() < MAXCONN && reqno < batch.size()) {
			SockClient *sock = new SockClient(this, addr, port);
			socks.append(sock);
			sock->request(batch[reqno++]);
			Q_ASSERT(!sock->done());
		}

		// The batch is done when we've garbage collected everything
		batchdone = socks.isEmpty();

	} else {
		// Using persistent connections (possibly pipelined).

		// Distribute requests on existing, idle connections first.
		for (int i = 0; i < socks.size(); i++) {
			if (reqno == batch.size())
				break;	// no more work in this batch
			if (!socks[i]->done())
				continue;
			socks[i]->request(batch[reqno++]);
		}

		// Launch new connections as allowed/needed,
		// at first sending only one request per connection.
		while (socks.size() < MAXCONN && reqno < batch.size()) {
			SockClient *sock = new SockClient(this, addr, port);
			socks.append(sock);
			sock->request(batch[reqno++]);
			Q_ASSERT(!sock->done());
		}

		// Now distribute further requests on existing connections
		// in round-robin fashion as their request queues permit.
		bool progress;
		do {
			progress = false;
			for (int i = 0; i < socks.size(); i++) {
				if (reqno == batch.size())
					break;	// no more work in this batch
				if (socks[i]->nreqs() >= PIPELEN)
					continue;	// this pipe full
				socks[i]->request(batch[reqno++]);
				progress = true;
			}
		} while (progress);

		// The batch is done when there's no activity on any socket.
		// (But we leave the sockets open for the next batch.)
		batchdone = true;
		for (int i = 0; i < socks.size(); i++)
			if (!socks[i]->done()) {
				batchdone = false;
				break;
			}
	}

	// When the batch completes, move on to the next
	if (batchdone) {
		Q_ASSERT(reqno == batch.size());
		reqno = 0;

		if (cp.primary && !cp.secondary.isEmpty()) {
			// primary done, now do secondary downloads
			cp.primary = 0;
			return check();
		} 

		// This page is complete!
		int nreqs = 1 + cp.secondary.size();
		int nreqlog = 0;
		while (nreqs > (1 << nreqlog) && nreqlog < 4)
			nreqlog++;
		qint64 elapsed = host->currentTime().usecs - pstart.usecs;
		logstrm << qSetFieldWidth(3) << "page" << nreqlog
			<< qSetFieldWidth(12) << totsize << elapsed
			<< qSetFieldWidth(3) << "   nreqs" << nreqs << endl;

		// Move to next page.
		cps.removeFirst();
		return check();
	}
}

void TestClient::priTimeout()
{
	printf("%lld priTimeout\n", host->currentTime().usecs);

	switch (prievent) {
	case 0: {
		// Start another parallel request, this one with high priority.
		SockClient *sock = new SockClient(this, addr, port);
		socks.append(sock);
		sock->request(3*1024*1024/2);
		Q_ASSERT(!sock->done());

		pritimer.start(5*1000*1000);
		prievent = 1;
		break; }

	case 1: {
		// Re-prioritize the streams
		socks[0]->reqPriChange(1);
		socks[2]->reqPriChange(0);
		break; }
	}
}

void TestClient::complete()
{
	qDebug() << "Testing complete!";
	qDebug() << "Total requests processed:" << count;

	QCoreApplication::exit(0);
}

