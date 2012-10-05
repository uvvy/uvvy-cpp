
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <QDir>
#include <QSettings>
#include <QHostInfo>
#include <QStringList>
#include <QCoreApplication>

#include "host.h"
#include "cli.h"

using namespace SST;


char *progname;
QSettings *privsettings;

void usage()
{
	fprintf(stderr, "Usage: %s <nickname> [<hostname> [<port>]]\n",
		progname);
	exit(1);
}

int main(int argc, char **argv)
{
	progname = argv[0];

	QCoreApplication app(argc, argv);
	app.setOrganizationName("MIT");
	app.setApplicationName("nsh");

	// Grab the required nickname argument
	if (argc < 2)
		usage();
	QString nickname(argv[1]);

	// Parse the optional hostname and port arguments
	QList<QHostAddress> addrs;
	int port = NETSTERIA_DEFAULT_PORT;
	if (argc >= 3) {
		// Parse the host name and port arguments
		QString hostname = argv[2];
		if (argc >= 4) {
			bool ok;
			port = QString(argv[3]).toInt(&ok);
			if (!ok || port <= 0 || port > 65535) {
				fprintf(stderr,
					"Port must be between 1 and 65535\n");
				usage();
			}
		}
		if (argc > 4)
			usage();

		// First see if the hostname is just an IP address
		QHostAddress addr(hostname);
		if (!addr.isNull()) {
			// Yes, it's an IP address - just use it.
			addrs.append(addr);
		} else {
			// No, try to resolve it as a DNS host name
			QHostInfo hi = QHostInfo::fromName(hostname);
			if (hi.error() != hi.NoError) {
				fprintf(stderr,
					"Can't find host '%s': %s\n",
					hostname.toLocal8Bit().data(),
					hi.errorString().toLocal8Bit().data());
				exit(1);
			}
			addrs = hi.addresses();
			Q_ASSERT(!addrs.isEmpty());
		}
	}

	// Convert the list of addresses to a list of endpoints,
	// using the same port number for each.
	QList<Endpoint> eps;
	foreach (const QHostAddress &addr, addrs)
		eps.append(Endpoint(addr, port));


	privsettings = new QSettings();

	// Initialize SST and read or create our own host identity
	Host host(privsettings, NETSTERIA_DEFAULT_PORT);


	// Find any existing information we have about the requested nickname.
	privsettings->beginGroup(QString("nickname:") + nickname);
	QByteArray eid = QByteArray::fromBase64(
			privsettings->value("eid").toString().toAscii());
	// XX address hints?
	privsettings->endGroup();

	if (eid.isNull() && eps.isEmpty()) {
		fprintf(stderr, "Host nickname '%s' not known: "
			"please specify host's DNS name or IP address.\n",
			nickname.toLocal8Bit().data());
		usage();
	}


	// Connect to the shell service
	ShellClient sc(&host);
	sc.connectTo(eid);

	// Register the list of target address hints
	foreach (const Endpoint &ep, eps)
		sc.connectAt(ep);

	// Set up the pseudo-tty on the server side, if appropriate.
	if (isatty(STDIN_FILENO))
		sc.setupTerminal(STDIN_FILENO);

	// Set up data forwarding
	QString cmd;	// XXX just a plain shell for now
	sc.runShell(cmd, STDIN_FILENO, STDOUT_FILENO);

	return app.exec();
}

