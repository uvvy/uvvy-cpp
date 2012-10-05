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

#include <QCoreApplication>
#include <QtDebug>

#include "sim.h"

using namespace SST;


// 56Kbps modem connection parameters
//static const int RATE = 53000/8; // 56Kbps = 53Kbps effective w/ overhead
//static const int DELAY = 200000/2; // uS ping

// DSL-level connection parameters
//static const int RATE = 1400000/8; // 1.5Mbps DSL connection
//static const int DELAY = 120000/2; // uS ping - typ for my DSL connection

// Low-delay DSL connection to MIT
//static const int RATE = 1500000/8; // 1.5Mbps DSL connection
//static const int DELAY = 55000/2; // uS ping - typ for my DSL connection

// 56Mbps 802.11 LAN
//static const int RATE = 56000000/8; // 56Mbps wireless LAN
//static const int DELAY = 750/2; // 750uS ping - typ for my DSL connection

//#define CUTOFF	(DELAY*10)	// max delay before we drop packets
#define PKTOH	32		// Bytes of link/inet overhead per packet


// Typical ADSL uplink/downlink bandwidths and combinations (all in Kbps)
// (e.g., see Dischinger et al, "Characterizing Residental Broadband Networks")
const int dsl_dn_bw[] = {128,256,384,512,768,1024,1536,2048,3072,4096,6144};
const int dsl_up_bw[] = {128,384,512,768};
const int dsl_bw[][2] =	// specific down/up combinations
	{{128,128},{384,128},{768,384},{1024,384},{1536,384},{2048,384},
	 {3072,512},{4096,512},{6144,768}};

#define DSL_DN_BW	1536	// Most common ADSL link speed in 2007
#define DSL_UP_BW	384

#define CABLE_DN_BW	5000	// Most common cable link speed
#define CABLE_UP_BW	384

// Typical SDSL/SHDSL bandwidth parameters (Kbps)
const int sdsl_bw[] = {512,1024,1536,2048,4096};


// Typical cable modem uplink/downlink bandwidths (all in Kbps)
const int cable_down_bw[] = {1500, 3000, 5000, 6000, 8000, 9000};
const int cable_up_bw[] = {250, 400, 500, 1000, 1500};


// Common downlink queue sizes in Kbytes, according to QFind results
// (Claypool et al, "Inferring Queue Sizes in Access Networks...")
const int dsl_dn_qsize[] = {10, 15, 25, 40, 55, 60};
const int cable_dn_qsize[] = {5, 10, 15, 20};


// Common measured last-hop link delays from Dischinger study:
// Note: round-trip, in milliseconds
const int dsl_delay[] = {7, 10, 13, 15, 20};
const int cable_delay[] = {5, 7, 10, 20};

#define DSL_RTDELAY	13	// approx median, milliseconds
#define CABLE_RTDELAY	7	// approx median, milliseconds


// Common downlink and uplink queue lengths in milliseconds,
// according to Dischinger et al study
const int dsl_dn_qlen[] = {30,90,130,200,250,300,350,400};
const int dsl_up_qlen[] = {50,250,750,1200,1700,2500};
const int cable_dn_qlen[] = {30,75,130,200,250};
const int cable_up_qlen[] = {100,800,1800,2200,2500,3000,4000};

#define DSL_DN_QLEN	300
#define DSL_UP_QLEN	750	// Very common among many ISPs
#define CABLE_DN_QLEN	130	// Very common among many ISPs
#define CABLE_UP_QLEN	2200


// Typical residential broadband DSL link
const LinkParams dsl15_dn =
	{ DSL_DN_BW*1024/8, DSL_RTDELAY*1000/2, DSL_DN_QLEN*1000 };
const LinkParams dsl15_up =
	{ DSL_UP_BW*1024/8, DSL_RTDELAY*1000/2, DSL_UP_QLEN*1000 };

// Typical residential cable modem link
const LinkParams cable5_dn =
	{ CABLE_DN_BW*1024/8, CABLE_RTDELAY*1000/2, CABLE_DN_QLEN*1000 };
const LinkParams cable5_up =
	{ CABLE_UP_BW*1024/8, CABLE_RTDELAY*1000/2, CABLE_UP_QLEN*1000 };


// Calculate transmission time of one packet in microseconds,
// given a packet size in bytes and transmission rate in bytes per second
#define	txtime(bytes,rate)	((qint64)(bytes) * 1000000 / (rate))

#define ETH10_RATE	(10*1024*1024/8)
#define ETH100_RATE	(100*1024*1024/8)
#define ETH1000_RATE	(1000*1024*1024/8)

#define ETH10_DELAY	(2000/2)	// XXX wild guess
#define ETH100_DELAY	(1000/2)	// XXX one data-point, YMMV
#define ETH1000_DELAY	(650/2)		// XXX one data-point, YMMV

#define ETH_MTU		1500	// Standard Ethernet MTU
#define ETH_QPKTS	25	// Typical queue length in packets (???)
#define ETH_QBYTES	(ETH_MTU * ETH_QPKTS)

// Ethernet link parameters (XXX are queue length realistic?)
const LinkParams eth10 =
	{ ETH10_RATE, ETH10_DELAY/2, txtime(ETH_QBYTES,ETH10_RATE) };
const LinkParams eth100 =
	{ ETH100_RATE, ETH100_DELAY/2, txtime(ETH_QBYTES,ETH100_RATE) };
const LinkParams eth1000 =
	{ ETH1000_RATE, ETH1000_DELAY/2, txtime(ETH_QBYTES,ETH1000_RATE) };


// Satellite link parameters (XXX need to check)
const LinkParams sat10 =
	{ ETH10_RATE, 500000, 1024*1024 };


static bool tracepkts = false;



////////// LinkParams //////////

QString LinkParams::toString()
{
	QString speed = rate < 1024*1024
		? QString("%1Kbps").arg((float)rate * 8 / (1024))
		: QString("%1Mbps").arg((float)rate * 8 / (1024*1024));
	return QString("%1 delay %2ms qlen %3ms")
		.arg(speed)
		.arg((float)delay / 1000)
		.arg((float)qlen / 1000);
}


////////// SimTimerEngine //////////

SimTimerEngine::SimTimerEngine(Simulator *sim, Timer *t)
:	TimerEngine(t), sim(sim), wake(-1)
{
}

SimTimerEngine::~SimTimerEngine()
{
	stop();
}

void SimTimerEngine::start(quint64 usecs)
{
	stop();

	wake = sim->cur.usecs + usecs;
	//qDebug() << "start timer for" << wake;

	int pos = 0;
	while (pos < sim->timers.size() && sim->timers[pos]->wake <= wake)
		pos++;
	sim->timers.insert(pos, this);
}

void SimTimerEngine::stop()
{
	if (wake < 0)
		return;

	//qDebug() << "stop timer at" << wake;
	sim->timers.removeAll(this);
	wake = -1;
}


////////// SimPacket //////////

SimPacket::SimPacket(SimHost *srch, const Endpoint &src, 
			SimLink *lnk, const Endpoint &dst,
			const char *data, int size)
:	QObject(srch->sim),
	sim(srch->sim),
	src(src), dst(dst), dsth(NULL),
	buf(data, size), timer(srch, this)
{
	Q_ASSERT(srch->links.value(src.addr) == lnk);

	// Find the destination on the appropriate incident link;
	// drop the packet if destination host not found.
	int w = lnk->which(srch);
	dsth = lnk->hosts[!w];
	if (!dsth || !(lnk->addrs[!w] == dst.addr)) {
		qDebug() << this << "target host"
			<< dst.addr.toString() << "not on specified link!";
		deleteLater();
		return;
	}

	qint64 curusecs = sim->currentTime().usecs;

	// Pick the correct set of link parameters to simulate with
	LinkParams &p = lnk->params[!w];
	qint64 &arr = lnk->arrival[!w];

	// Simulate random loss if appropriate
	if (p.loss && drand48() <= p.loss) {
		qDebug() << this << "random DROP";
		deleteLater();
		return;
	}

	// Earliest time packet could start to arrive based on network delay
	qint64 nomarr = curusecs + p.delay;

	// Compute the time the packet's first bit will actually arrive -
	// it can't start arriving sooner than the last packet finished.
	qint64 actarr = qMax(nomarr, arr);

	// If the computed arrival time is too late, drop this packet.
	// Implement a standard, basic drop-tail policy.
	bool drop = actarr > nomarr + p.qlen;

	// Compute the amount of wire time this packet takes to transmit,
	// including some per-packet link/inet overhead
	qint64 psize = buf.size() + PKTOH;
	qint64 ptime = psize * 1000000 / p.rate;

	quint32 seqno = ntohl(*(quint32*)buf.data()) & 0xffffff;
	isclient = !w;	// XXX hacke
	if (tracepkts)
	if (isclient)
		qDebug("%12lld:\t\t     --> %6d %4d --> %4s  @%lld",
			curusecs, seqno, buf.size(), drop ? "DROP" : "",
			drop ? 0 : actarr + ptime);
	else
		qDebug("%12lld:\t\t\t\t%4s <-- %6d %4d <--  @%lld",
			curusecs, drop ? "DROP" : "", seqno, buf.size(),
			drop ? 0 : actarr + ptime);

	if (drop) {
		qDebug() << "SimPacket: DROP";
		deleteLater();
		return;
	}

	// Finally, record the time the packet will finish arriving,
	// and schedule the packet to arrive at that time.
	arr = actarr + ptime;

	// Add it to the host's receive packet queue
	dsth->pqueue.append(this);

	connect(&timer, SIGNAL(timeout(bool)), this, SLOT(arrive()));
	timer.start(arr - curusecs);
}

void SimPacket::arrive()
{
	// Make sure we're still on the destination host's queue
	if (!dsth || !dsth->pqueue.contains(this)) {
		qDebug() << this << "no longer queued to destination host"
			<< dst.addr.toString();
		return deleteLater();
	}

	timer.stop();

	quint32 seqno = ntohl(*(quint32*)buf.data()) & 0xffffff;
	if (tracepkts)
	if (isclient)
		qDebug("%12lld:\t\t\t\t\t\t        --> %6d %4d",
			sim->currentTime().usecs, seqno, buf.size());
	else
		qDebug("%12lld: %6d %4d <--",
			sim->currentTime().usecs, seqno, buf.size());

	SimSocket *dsts = dsth->socks.value(dst.port);
	if (!dsts) {
		qDebug() << this << "target has no listener on port"
			<< dst.port;
		return deleteLater();
	}

	// Remove this packet from the destination host's queue
	dsth->pqueue.removeAll(this);
	dsth = NULL;

	SocketEndpoint sep(src, dsts);
	dsts->receive(buf, sep);

	deleteLater();
}

SimPacket::~SimPacket()
{
	if (dsth)
		dsth->pqueue.removeAll(this);
}


////////// SimSocket //////////

SimSocket::SimSocket(SimHost *host, QObject *parent)
:	Socket(host, parent),
	sim(host->sim),
	host(host),
	port(0)
{
}

SimSocket::~SimSocket()
{
	unbind();
}

bool SimSocket::bind(const QHostAddress &addr, quint16 port,
			QUdpSocket::BindMode)
{
	Q_ASSERT(!this->port);
	Q_ASSERT(addr == QHostAddress::Any);

	if (port == 0) {
		for (port = 1; host->socks.contains(port); port++)
			Q_ASSERT(port < 65535);
	}

	Q_ASSERT(!host->socks.contains(port));
	host->socks.insert(port, this);
	this->port = port;

	//qDebug() << "Bound virtual socket on host" << host->addr.toString()
	//	<< "port" << port;

	setActive(true);
	return true;
}

void SimSocket::unbind()
{
	if (port) {
		Q_ASSERT(host->socks.value(port) == this);
		host->socks.remove(port);
		port = 0;
		setActive(false);
	}
}

bool SimSocket::send(const Endpoint &dst, const char *data, int size)
{
	Q_ASSERT(port);

	// Find the neighbor host and link given the destination address
	Endpoint src;
	src.port = port;
	SimHost *dsth = host->neighborAt(dst.addr, src.addr);
	if (!dsth) {
		qDebug() << this << "unknown or un-adjacent target host"
			<< dst.addr.toString();
		return false;
	}

	SimLink *link = host->linkAt(src.addr);
	Q_ASSERT(link);

	(void)new SimPacket(host, src, link, dst, data, size);
	return true;
}

QList<Endpoint> SimSocket::localEndpoints()
{
	Q_ASSERT(port);

	QList<Endpoint> l;
	foreach (const QHostAddress &addr, host->links.keys()) {
		l.append(Endpoint(addr, port));
	}
	return l;
}

quint16 SimSocket::localPort()
{
	return port;
}

QString SimSocket::errorString()
{
	return QString();	// XXX
}


////////// SimHost //////////

SimHost::SimHost(Simulator *sim)
:	sim(sim)
{
	initSocket(NULL);

	// expensive, and can be done lazily if we need a cryptographic ID...
	//initHostIdent(NULL);
}

SimHost::~SimHost()
{
	// Unbind all sockets from this host
	foreach (quint16 port, socks.keys())
		socks[port]->unbind();

	// Detach this host from all network links
	foreach (const QHostAddress addr, links.keys()) {
		links.value(addr)->disconnect();
	}
	Q_ASSERT(links.empty());

	// Detach and delete all packets queued to this host
	foreach (SimPacket *pkt, pqueue) {
		Q_ASSERT(pkt->dsth == this);
		pkt->dsth = NULL;
		pkt->deleteLater();
	}
}

Time SimHost::currentTime()
{
	return sim->realtime ? Host::currentTime() : sim->cur;
}

TimerEngine *SimHost::newTimerEngine(Timer *timer)
{
	return sim->realtime
		? Host::newTimerEngine(timer)
		: new SimTimerEngine(sim, timer);
}

Socket *SimHost::newSocket(QObject *parent)
{
	return new SimSocket(this, parent);
}

SimHost *SimHost::neighborAt(const QHostAddress &dstaddr, QHostAddress &srcaddr)
{
	QHashIterator<QHostAddress, SimLink*> i(links);
	while (i.hasNext()) {
		i.next();
		SimLink *l = i.value();
		int w = l->which(this);
		Q_ASSERT(l->addrs[w] == i.key());
		if (l->addrs[!w] == dstaddr) {
			srcaddr = i.key();
			return l->hosts[!w];
		}
	}
	return NULL;	// not found
}


////////// SimLink //////////

SimLink::SimLink(LinkPreset preset)
{
	hosts[0] = hosts[1] = NULL;
	arrival[0] = arrival[1] = 0;

	setPreset(preset);
}

SimLink::~SimLink()
{
	disconnect();
}

void SimLink::setPreset(LinkPreset preset)
{
	switch (preset) {
	case DSL15:	setLinkParams(dsl15_dn, dsl15_up); break;
	case Cable5:	setLinkParams(cable5_dn, cable5_up); break;
	case Sat10:	setLinkParams(sat10); break;
	case Eth10:	setLinkParams(eth10); break;
	case Eth100:	setLinkParams(eth100); break;
	case Eth1000:	setLinkParams(eth1000); break;
	default:
		qFatal("SimLink: unknown preset %d", preset);
	}
}

void SimLink::setLinkParams(const LinkParams &down, const LinkParams &up)
{
	params[0] = down;
	params[1] = up;

	qDebug() << this << "downlink:" << params[0].toString();
	qDebug() << this << "uplink:" << params[1].toString();
}

void SimLink::connect(SimHost *downHost, const QHostAddress &downAddr,
		SimHost *upHost, const QHostAddress &upAddr)
{
	Q_ASSERT(upHost != downHost);
	Q_ASSERT(upAddr != downAddr);
	Q_ASSERT(hosts[0] == NULL);
	Q_ASSERT(hosts[1] == NULL);

	hosts[0] = downHost;
	addrs[0] = downAddr;
	hosts[1] = upHost;
	addrs[1] = upAddr;

	Q_ASSERT(downHost->links.value(downAddr) == NULL);
	downHost->links.insert(downAddr, this);

	Q_ASSERT(upHost->links.value(upAddr) == NULL);
	upHost->links.insert(upAddr, this);
}

void SimLink::disconnect()
{
	for (int i = 0; i < 2; i++) {
		if (hosts[i]) {
			Q_ASSERT(hosts[i]->links.value(addrs[i]) == this);
			hosts[i]->links.remove(addrs[i]);
			hosts[i] = 0;
		}
	}
}



////////// Simulator //////////

Simulator::Simulator(bool realtime)
:	realtime(realtime)
{
	cur.usecs = 0;
}

Simulator::~Simulator()
{
	//qDebug() << "~" << this;
	// Note that there may still be packets in the simulated network,
	// and simulated timers representing their delivery time -
	// but they should all get garbage collected at this point.
}

Time Simulator::currentTime()
{
	if (realtime)
		return Time::fromQDateTime(QDateTime::currentDateTime());
	else
		return cur;
}

void Simulator::run()
{
	//qDebug() << "Simulator::run()";
	if (realtime)
		qFatal("Simulator::run() is only for use with virtual time:\n"
			"for real time, use QCoreApplication::exec() instead.");

	while (!timers.isEmpty()) {
		SimTimerEngine *next = timers.dequeue();
		Q_ASSERT(next->wake >= cur.usecs);

		// Move the virtual system clock forward to this event
		cur.usecs = next->wake;
		next->wake = -1;

		// Dispatch the event
		next->timeout();

		// Process any Qt events such as delayed signals
		QCoreApplication::processEvents(QEventLoop::DeferredDeletion);

		//qDebug() << "";	// print blank lines at time increments
		eventStep();		// notify interested listeners
	}
	//qDebug() << "Simulator::run() done";
}

