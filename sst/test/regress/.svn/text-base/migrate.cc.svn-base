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

#include <stdlib.h>

#include <QtDebug>

#include "main.h"
#include "migrate.h"

using namespace SST;


#define MIGRBYTES	(10*1024*1024)	// transfer min 1MB between migrs
//#define MIGRTIME	(10*1000000)	// time between migrations

//#define MAXMSGS		1000

#define MAXMIGRS	10


#define MINP2	0	// Minimum message size power-of-two
#define MAXP2	16	// Minimum message size power-of-two

//#define MINP2	28	// Minimum message size power-of-two
//#define MAXP2	28	// Minimum message size power-of-two


// Time period for bandwidth sampling
//#define TICKTIME	10000


MigrateTest::MigrateTest()
:	clihost(&sim),
	srvhost(&sim),
	cli(&clihost),
	srv(&srvhost),
	srvs(NULL),
	migrater(&clihost),
	narrived(0), nmigrates(0),
	lastmigrtime(0),
	totarrived(0), lasttotarrived(0), smoothrate(0),
	ticker(&clihost)
{
	// Gather simulation statistics
	//connect(&sim, SIGNAL(eventStep()), this, SLOT(gotEventStep()));

	curaddr = cliaddr;
	link.connect(&clihost, curaddr, &srvhost, srvaddr);
	link.setPreset(Eth10);

	starttime = clihost.currentTime();
	connect(&migrater, SIGNAL(timeout(bool)), this, SLOT(gotTimeout()));

	connect(&srv, SIGNAL(newConnection()),
		this, SLOT(gotConnection()));
	if (!srv.listen("regress", "SST regression test server",
			"migrate", "Migration test protocol"))
		qFatal("Can't listen on service name");

	// Open a connection to the server
	connect(&cli, SIGNAL(readyRead()), this, SLOT(gotData()));
	connect(&cli, SIGNAL(readyReadMessage()), this, SLOT(gotMessage()));
	cli.connectTo(srvhost.hostIdent(), "regress", "migrate");
	cli.connectAt(Endpoint(srvaddr, NETSTERIA_DEFAULT_PORT));

#ifdef MIGRTIME
	// If migrating purely periodically, start the timer
	migrater.start(MIGRTIME);
#endif

#ifdef TICKTIME
	connect(&ticker, SIGNAL(timeout(bool)), this, SLOT(gotTick()));
	ticker.start(TICKTIME);
#endif

	// Get the ping-pong process going...
	ping(&cli);
}

void MigrateTest::gotConnection()
{
	qDebug() << this << "gotConnection";
	Q_ASSERT(srvs == NULL);

	srvs = srv.accept();
	if (!srvs) return;

	srvs->listen(Stream::Unlimited);

	connect(srvs, SIGNAL(readyRead()), this, SLOT(gotData()));
	connect(srvs, SIGNAL(readyReadMessage()), this, SLOT(gotMessage()));
}

void MigrateTest::ping(Stream *strm)
{
	// Send a random-size message
	int p2 = (MAXP2 > MINP2)
			? (lrand48() % (MAXP2-MINP2)) + MINP2
			: MAXP2;
	QByteArray buf;
	buf.resize(1 << p2);
	//qDebug() << strm << "send msg size" << buf.size();
	strm->writeMessage(buf);
}

void MigrateTest::gotData()
{
	Stream *strm = (Stream*)QObject::sender();
	while (true) {
		QByteArray buf = strm->readData();
		if (buf.isEmpty())
			return;

		arrived(buf.size());
		qDebug() << strm << "recv size" << buf.size()
			<< "count" << narrived << "/" << MIGRBYTES;
	}
}

void MigrateTest::gotMessage()
{
	Stream *strm = (Stream*)QObject::sender();
	while (true) {
		QByteArray buf = strm->readMessage();
		if (buf.isNull())
			return;

		arrived(buf.size());
		qDebug() << strm << "recv msg size" << buf.size()
			<< "count" << narrived << "/" << MIGRBYTES;

		// Keep ping-ponging until we're done.
		if (nmigrates < MAXMIGRS)
			ping(strm);
	}
}

void MigrateTest::arrived(int amount)
{
	// Count arrived data and use it to time the next migration.
	int newarrived = narrived + amount;
	if (narrived < MIGRBYTES && newarrived >= MIGRBYTES) {
		timeperiod = clihost.currentTime().usecs
			- starttime.usecs;
		qDebug() << "migr" << nmigrates << "timeperiod:"
			<< (double)timeperiod / 1000000.0;

#ifndef MIGRTIME
		// Wait some random addional time before migrating,
		// to ensure that migration can happen at
		// "unexpected moments"...
		migrater.start(timeperiod * drand48());
#endif
	}
	narrived = newarrived;

	totarrived += amount;
}

void MigrateTest::gotTimeout()
{
	// Find the next IP address to migrate to
	quint32 newip = curaddr.toIPv4Address() + 1;
	if (newip == srvaddr.toIPv4Address())
		newip++;	// don't use same addr as server!
	QHostAddress newaddr = QHostAddress(newip);

	// Migrate!
	qDebug() << "migrating to" << newaddr;
	link.disconnect();
	link.connect(&clihost, newaddr, &srvhost, srvaddr);
	curaddr = newaddr;
	srvs->connectAt(Endpoint(newaddr, NETSTERIA_DEFAULT_PORT));

	// Start the next cycle...
	starttime = clihost.currentTime();
	narrived = 0;
	nmigrates++;

#ifdef MIGRTIME
	// If migrating purely periodically, start the timer
	migrater.start(MIGRTIME);
#endif
}

void MigrateTest::gotTick()
{
#ifdef TICKTIME
	// Restart the timer
	ticker.start(TICKTIME);

	long long difftime = sim.currentTime().usecs - lasttime;
	int diffbytes = totarrived - lasttotarrived;
	if (difftime == 0)
		return;

	float instrate = (float)diffbytes / difftime * 1000000.0;
	smoothrate = (smoothrate * 9 + instrate) / 10;

	printf("%f %f %lld %d %f %f\n",
		(float)sim.currentTime().usecs/1000000.0,
		(float)difftime/1000000.0,
		totarrived, diffbytes, instrate, smoothrate);

	lasttime = sim.currentTime().usecs;
	lasttotarrived = totarrived;
#endif
}

void MigrateTest::run()
{
	MigrateTest test;
	test.sim.run();

	qDebug() << "Migrate test completed after" << test.nmigrates;
	success = true;
	check(test.nmigrates == MAXMIGRS);
}

