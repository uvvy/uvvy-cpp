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
#ifndef SIM_H
#define SIM_H

#include <QHash>
#include <QQueue>
#include <QHostAddress>

#include "host.h"

namespace SST {

class SimHost;
class SimLink;
class Simulator;

struct LinkParams {
	int rate;	// Bandwidth in bytes per second
	int delay;	// Link delay in microseconds
	int qlen;	// Router queue length in microseconds
	float loss;	// Random loss rate, form 0.0 to 1.0

	QString toString();
};

enum LinkPreset {
	DSL15,		// 1.5Mbps/384Kbps DSL link
	Cable5,		// 5Mbps cable modem link
	Sat10,		// 10Mbps satellite link with 500ms delay
	Eth10,		// 10Mbps Ethernet link
	Eth100,		// 100Mbps Ethernet link
	Eth1000,	// 1000Mbps Ethernet link
};

class SimTimerEngine : public TimerEngine
{
	friend class SimHost;
	friend class Simulator;

private:
	Simulator *sim;
	qint64 wake;

protected:
	SimTimerEngine(Simulator *sim, Timer *t);
	~SimTimerEngine();

	virtual void start(quint64 usecs);
	virtual void stop();
};

class SimPacket : public QObject
{
	friend class SimHost;
	Q_OBJECT

private:
	Simulator *sim;
	Endpoint src, dst;
	SimHost *dsth;
	QByteArray buf;
	Timer timer;
	bool isclient;

public:
	SimPacket(SimHost *srch, const Endpoint &src, 
			SimLink *link, const Endpoint &dst,
			const char *data, int size);
	~SimPacket();

private slots:
	void arrive();
};

class SimSocket : public Socket
{
	friend class SimPacket;
	Q_OBJECT

private:
	Simulator *const sim;
	SimHost *const host;
	quint16 port;

public:
	SimSocket(SimHost *h, QObject *parent = NULL);
	~SimSocket();

	virtual bool bind(const QHostAddress &addr = QHostAddress::Any,
		quint16 port = 0,
		QUdpSocket::BindMode mode = QUdpSocket::DefaultForPlatform);
	virtual void unbind();

	virtual bool send(const Endpoint &ep, const char *data, int size);
	virtual QList<Endpoint> localEndpoints();
	virtual quint16 localPort();
	virtual QString errorString();
};

class SimHost : public Host
{
	friend class SimTimerEngine;
	friend class SimPacket;
	friend class SimSocket;
	friend class SimLink;

private:
	Simulator *sim;

	// Virtual network links to which this host is attached
	QHash<QHostAddress, SimLink*> links;

	// Sockets bound on this host
	QHash<quint16, SimSocket*> socks;

	// Queue of packets to be delivered to this host
	QList<SimPacket*> pqueue;

public:
	SimHost(Simulator *sim);
	~SimHost();

	virtual Time currentTime();
	virtual TimerEngine *newTimerEngine(Timer *timer);

	virtual Socket *newSocket(QObject *parent = NULL);


	// Return this simulated host's current set of IP addresses.
	inline QList<QHostAddress> hostAddresses() const {
		return links.keys(); }

	// Return the currently attached link at a particular address,
	// NULL if no link attached at that address.
	inline SimLink *linkAt(const QHostAddress &addr) {
		return links.value(addr); }

	// Find a neighbor host on some adjacent link
	// by the host's IP address on that link.
	// Returns the host pointer, or NULL if not found.
	// If found, also sets srcaddr to this host's address
	// on the network link on which the neighbor is found.
	SimHost *neighborAt(const QHostAddress &dstaddr, QHostAddress &srcaddr);
};

class SimLink
{
	friend class SimHost;
	friend class SimPacket;

private:
	Simulator *sim;

	// Hosts attached to this link.
	// For asymmetric links, the first is "down", the second "up".
	SimHost *hosts[2];

	// Address at which each host is attached to this link
	QHostAddress addrs[2];

	// Network performance settings: 0=down, 1=up
	LinkParams params[2];

	// Minimum network arrival time for next packet to be received
	qint64 arrival[2];

	inline int which(SimHost *h) {
		Q_ASSERT(h == hosts[0] || h == hosts[1]);
		return h == hosts[1];
	}

public:

	SimLink(LinkPreset preset = Eth100);
	~SimLink();

	// Connect two hosts via this link
	void connect(SimHost *downHost, const QHostAddress &downAddr,
			SimHost *upHost, const QHostAddress &upAddr);

	// Disconnect this link
	void disconnect();

	inline LinkParams downLinkParams() const { return params[0]; }
	inline LinkParams upLinkParams() const { return params[1]; }

	void setPreset(LinkPreset preset);

	void setLinkParams(const LinkParams &down, const LinkParams &up);
	inline void setLinkParams(const LinkParams &params)
		{ setLinkParams(params, params); }

	// Set link rate in bytes per second
	inline void setLinkRate(int down, int up)
		{ params[0].rate = down; params[1].rate = up; }
	inline void setLinkRate(int bps) { setLinkRate(bps, bps); }

	// Set link delay in microseconds
	inline void setLinkDelay(int down, int up)
		{ params[0].delay = down; params[1].delay = up; }
	inline void setLinkDelay(int usecs) { setLinkDelay(usecs, usecs); }

	// Set queue length in microseconds
	inline void setLinkQueue(int down, int up)
		{ params[0].qlen = down; params[1].qlen = up; }
	inline void setLinkQueue(int usecs) { setLinkQueue(usecs, usecs); }

	// Set link's random loss rate as a fraction between 0.0 and 1.0.
	inline void setLinkLoss(float down, float up)
		{ params[0].loss = down; params[1].loss = up; }
	inline void setLinkLoss(float loss) { setLinkLoss(loss, loss); }
};

class Simulator : public QObject
{
	friend class SimTimerEngine;
	friend class SimPacket;
	friend class SimSocket;
	friend class SimHost;
	Q_OBJECT

private:
	// True if we're using realtime instead of virtualized time
	const bool realtime;

	// The current virtual system time
	Time cur;

	// List of all currently active timers sorted by wake time
	QQueue<SimTimerEngine*> timers;

	// Table of all hosts in the simulation
	//QHash<QHostAddress, SimHost*> hosts;

public:
	Simulator(bool realtime = false);
	virtual ~Simulator();

	Time currentTime();

	void run();

signals:
	// The simulator emits this signal after each event processing step,
	// but before waiting for the next event to occur.
	// This can be hooked to output state statistics after each step.
	void eventStep();
};

} // namespace SST

#endif	// SIM_H
