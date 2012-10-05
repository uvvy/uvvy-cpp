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
#include <QCoreApplication>
#include <QtDebug>

#include "sock.h"
#include "host.h"

#include "main.h"
#include "cli.h"
#include "srv.h"
#include "sim.h"
#include "dgram.h"
#include "migrate.h"
#include "seg.h"

using namespace SST;


QHostAddress SST::cliaddr("1.2.3.4");
QHostAddress SST::srvaddr("4.3.2.1");

bool SST::success;
int SST::nerrors;


struct RegressionTest {
	void (*run)();
	const char *name;
	const char *descr;
} tests[] = {
	{BasicClient::run, "basic", "Basic stream-oriented data transfer"},
	{DatagramTest::run, "dgram", "Best-effort datagram data transfer"},
	{MigrateTest::run, "migrate", "Endpoint migration test"},
	{SegTest::run, "seg", "Segmented path test"},
};
#define NTESTS ((int)(sizeof(tests)/sizeof(tests[0])))

void usage(const char *appname)
{
	fprintf(stderr, "Usage: %s [<testname>]   - to run a specific test\n"
			"   or: %s all            - to run all tests\n"
			"Tests:\n", appname, appname);
	for (int i = 0; i < NTESTS; i++)
		fprintf(stderr, "  %-10s %s\n", tests[i].name, tests[i].descr);
	fprintf(stderr, "Runs all tests if invoked without an argument.\n");
	exit(1);
}

bool runtest(int n)
{
	RegressionTest &t = tests[n];

	printf("Running test '%s': ", t.name);
	fflush(stdout);

	success = false;
	nerrors = 0;

	t.run();

	bool succ = success && (nerrors == 0);
	printf(succ ? "passed\n" : "FAILED\n");

	return succ;
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	if (argc < 2) {
		usage(argv[0]);
	} else if (strcasecmp(argv[1], "all") == 0) {

		// Run all tests in succession.
		printf("Running all %d regression tests:\n", NTESTS);
		int nfail = 0;
		for (int i = 0; i < NTESTS; i++) {
			if (!runtest(i))
				nfail++;
		}

		// Print summary of results
		if (nfail) {
			printf("%d of %d tests FAILED\n", nfail, NTESTS);
			exit(1);
		} else
			printf("All tests passed.\n");

	} else if (argc == 2) {

		// Find and run the named test.
		for (int i = 0; ; i++) {
			if (i >= NTESTS)
				usage(argv[0]);	// Test not found
			if (strcasecmp(argv[1], tests[i].name) != 0)
				continue;

			// Run the test
			if (!runtest(i))
				exit(1);
			break;
		}

	} else
		usage(argv[0]);

	return 0;
}

