
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <QDir>
#include <QSettings>
#include <QCoreApplication>

#include "host.h"
#include "srv.h"


QSettings *privsettings;

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	app.setOrganizationName("MIT");
	app.setApplicationName("nshd");

	QString nshdir(QDir::home().path() + "/.nsh");
	if (getuid() == 0)
		nshdir = ETCDIR "/nsh";
	QDir().mkdir(nshdir);

	// Read or create the private settings file
	QString privname = nshdir + "/private";
	privsettings = new QSettings(nshdir + "/private", QSettings::IniFormat);
	privsettings->setValue("dummy", "");
	privsettings->sync();
	if (privsettings->status() != QSettings::NoError)
		qFatal("Can't access %s", privname.toUtf8().data());

	// Make sure the private settings file is really private
	if (chmod(privname.toUtf8().data(), 0600) < 0)
		qFatal("Can't chmod %s: %s", privname.toUtf8().data(),
			strerror(errno));

	// Initialize SST and read or create our host identity
	SST::Host host(privsettings, NETSTERIA_DEFAULT_PORT);

	// Create and register the shell service
	ShellServer svc(&host);

	return app.exec();
}

