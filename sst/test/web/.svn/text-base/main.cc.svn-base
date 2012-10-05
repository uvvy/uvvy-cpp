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

#include <string.h>
#include <stdlib.h>

#include <QHostInfo>
#include <QHostAddress>
#include <QApplication>
#include <QtDebug>

#include "sock.h"
#include "host.h"

#include "main.h"
#include "cli.h"
#include "srv.h"
#include "sim.h"

using namespace SST;


void usage(const char *progname)
{
	fprintf(stderr, "Usage:\n"
		" %s <host> [<port>]  - run client, connect to given server\n"
		" %s server [<port>]  - run server\n"
		" %s sim              - run client & server in simulation\n",
		progname, progname, progname);
	exit(1);
}

int client(const QApplication &app, const char *srvhost, int srvport)
{
	QHostAddress srvaddr(srvhost);
	if (srvaddr.isNull()) {
		QHostInfo hostinfo = QHostInfo::fromName(srvhost);
		QList<QHostAddress> addrs = hostinfo.addresses();
		if (addrs.size() <= 0)
			qFatal("Error resolving host '%s'", srvhost);
		srvaddr = addrs[0];
	}

	Host clihost(NULL, srvport);
	WebClient cli(&clihost, srvaddr, srvport);
	cli.show();

	return app.exec();
}

int server(const QApplication &app, int port)
{
	Host srvhost(NULL, port);
	WebServer srv(&srvhost, port);

	return app.exec();
}

int sim(const QApplication &app)
{
	QHostAddress cliaddr("1.2.3.4");
	QHostAddress srvaddr("4.3.2.1");

	Simulator sim(true);
	SimLink link;
	SimHost clihost(&sim);
	SimHost srvhost(&sim);
	link.connect(&clihost, cliaddr, &srvhost, srvaddr);

	WebServer srv(&srvhost, defaultPort);

	WebClient cli(&clihost, srvaddr, defaultPort, &link);
	cli.show();

	return app.exec();
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);

	if (argc < 2)
		usage(argv[0]);

	if (strcasecmp(argv[1], "sim") == 0) {
		if (argc > 2)
			usage(argv[0]);
		return sim(app);
	}

	// Second argument, if it exists, is port number
	int port = defaultPort;
	if (argc > 2) {
		bool ok;
		port = QString(argv[2]).toInt(&ok);
		if (!ok || port <= 0 || port > 65535)
			usage(argv[0]);
	}
	if (argc > 3)
		usage(argv[0]);

	if (strcasecmp(argv[1], "server") == 0) {
		if (argc > 2)
			usage(argv[0]);
		return server(app, port);
	}

	// Client mode
	return client(app, argv[1], port);
}

