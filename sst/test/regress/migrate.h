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
#ifndef MIGRATE_H
#define MIGRATE_H

#include "stream.h"
#include "sim.h"

class QIODevice;


namespace SST {

class Host;


class MigrateTest : public QObject
{
	Q_OBJECT

private:
	Simulator sim;
	SimLink link;
	SimHost clihost;
	SimHost srvhost;
	Stream cli;
	StreamServer srv;
	Stream *srvs;
	QHostAddress curaddr;

	Time starttime;
	quint64 timeperiod;
	Timer migrater;

	int narrived;
	int p2;
	int nmigrates;

	long long lastmigrtime;

	long long lasttime;
	long long totarrived, lasttotarrived;
	double smoothrate;

	Timer ticker;

public:
	MigrateTest();

	static void run();

private:
	void ping(Stream *strm);
	void arrived(int amount);

private slots:
	void gotConnection();
	void gotData();
	void gotMessage();
	void gotTimeout();
	void gotTick();
};


} // namespace SST

#endif	// MIGRATE_H
