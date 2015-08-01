#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QDir>
#include "host.h"
#include "peer.h"

using namespace SST;

//=====================================================================================================================
// Globals (yuck!)
//=====================================================================================================================

Host *ssthost;
RegInfo myreginfo;
QSettings *settings;
QFile logfile;
QDir appdir;

//=====================================================================================================================
// Qt logger function.
//=====================================================================================================================

void myMsgHandler(QtMsgType type, const char *msg)
{
    QTextStream strm(&logfile);
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss.zzz"); // ISO 8601 date with milliseconds
    switch (type) {
        case QtDebugMsg:
            strm << now << " D: " << msg << '\n';
            // std::cout << now << " D: " << msg << '\n';
            break;
        case QtWarningMsg:
            strm << now << " W: " << msg << '\n';
            // std::cout << now << " W: " << msg << '\n';
            break;
        case QtCriticalMsg:
            strm << now << " C: " << msg << '\n';
            strm.flush();
            // std::cout << now << " C: " << msg << '\n';
            break;
        case QtFatalMsg:
            strm << now << " F: " << msg << '\n';
            strm.flush();
            // std::cout << now << " F: " << msg << '\n';
            abort();
    }
}

//=====================================================================================================================
// Helper functions.
//=====================================================================================================================

static void usage()
{
    qCritical("Usage: sstreams [-i]\n"
        "  -i              Enable initiator mode, otherwise act as responder.\n");
    exit(1);
}

//=====================================================================================================================
// Main entry point.
//=====================================================================================================================

bool initiator = false;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("Exquance.com");
    app.setOrganizationDomain("exquance.com"); // for OSX
    app.setApplicationName("SstreamS");

    while (argc > 1 and argv[1][0] == '-') {
        switch (argv[1][1]) {
        case 'i':
            initiator = true;
            break;
        default:
            usage();
        }
        argc--, argv++;
    }

    QDir homedir = QDir::home();
    QString homedirpath = homedir.path();

    QString appdirname = ".netsteria";
    homedir.mkdir(appdirname);
    appdir.setPath(homedirpath + "/" + appdirname);

    settings = new QSettings;

    // Send debugging output to a log file
    QString logname(appdir.path() + "/sstreams.log");
    QString logbakname(appdir.path() + "/sstreams.log.bak");
    QFile::remove(logbakname);
    QFile::rename(logname, logbakname);
    logfile.setFileName(logname);
    if (!logfile.open(QFile::WriteOnly | QFile::Truncate))
        qWarning("Can't open log file '%s'",
            logname.toLocal8Bit().data());
    else
        qInstallMsgHandler(myMsgHandler);
    qDebug() << "SstreamS starting";

    // Initialize the Structured Stream Transport
    ssthost = new Host(settings, initiator ? 8999 : 9001);

    myreginfo.setHostName(initiator ? "EID1" : "EID2");
    myreginfo.setOwnerName(initiator ? "initiator" : "responder");
    myreginfo.setEndpoints(ssthost->activeLocalEndpoints());
    qDebug() << "local endpoints" << myreginfo.endpoints().size();

    RegClient *regcli = new RegClient(ssthost);
    regcli->setInfo(myreginfo);
    regcli->registerAt("ishikawa.local");

    /*PeerService* s =*/ new PeerService("test", QObject::tr("Testing"),
                                     "test", QObject::tr("Shmesting"));
    // connect(s, SIGNAL(inStreamConnected(Stream*)),
        // this, SLOT(gotInStreamConnected(Stream*)));

    if (initiator) {
        // try to connect to responder's test/test service
        // s->connectTo("EID2");
    }
    else {
        // listen and accept on test/test
    }

    int r = app.exec();

    // Disconnect from regservers
    delete regcli;

    return r;
}
