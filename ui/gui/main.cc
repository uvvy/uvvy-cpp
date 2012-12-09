#define VERSION "0.03"

#include <QHash>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QTableView>
#include <QHeaderView>
#include <QPushButton>
#include <QStringListModel>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QSettings>
#include <QHostInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QtDebug>

#ifdef MAC_OS_X_VERSION_10_6
#include "macsupport.h"
#endif

#include "arch.h"
#include "env.h"
#include "main.h"
#include "voice.h"
#include "chat.h"
#include "search.h"
#include "regcli.h"
#include "peer.h"
#include "chunk.h"
#include "share.h"
#include "save.h"
#include "host.h"
#include "settings.h"
#include "filesync.h"
#include "logwindow.h"

#include "upnp/upnpmcastsocket.h"
#include "upnp/router.h"

using namespace SST;

//=====================================================================================================================
// Globals (yuck!)
//=====================================================================================================================

Host *ssthost;

MainWindow *mainwin;
PeerTable *friends;
VoiceService *talksrv;

QList<RegClient*> regclients;
RegInfo myreginfo;

QSettings *settings;
QDir appdir;
QDir shareDir;
QFile logfile;

bool spewdebug;

volatile int finished_punch = 0;

//=====================================================================================================================
// Qt logger function.
//=====================================================================================================================

void myMsgHandler(QtMsgType type, const char *msg)
{
    QTextStream strm(&logfile);
    switch (type) {
        case QtDebugMsg:
            strm << "D: " << msg << '\n';
            LogWindow::get() << msg;
            if (spewdebug)
                std::cout << msg << '\n';
            break;
        case QtWarningMsg:
            strm << "W: " << msg << '\n';
            std::cout << "Warning: " << msg << '\n';
            LogWindow::get() << msg;
            break;
        case QtCriticalMsg:
            strm << "C: " << msg << '\n';
            strm.flush();
            std::cout << "Critical: " << msg << '\n';
            LogWindow::get() << msg;
            QMessageBox::critical(NULL,
                QObject::tr("Netsteria: Critical Error"), msg,
                QMessageBox::Ok, QMessageBox::NoButton);
            break;
        case QtFatalMsg:
            strm << "F: " << msg << '\n';
            strm.flush();
            std::cout << "Fatal: " << msg << '\n';
            LogWindow::get() << msg;
            QMessageBox::critical(NULL,
                QObject::tr("Netsteria: Critical Error"), msg,
                QMessageBox::Ok, QMessageBox::NoButton);
            abort();
    }
}

//=====================================================================================================================
// MainWindow
//=====================================================================================================================

MainWindow::MainWindow()
:   searcher(NULL)
{
    QIcon appicon(":/img/mettanode.png");

    setWindowTitle(tr("Metta node"));
    setWindowIcon(appicon);

    // Create a ListView onto our friends list, as the central widget
    Q_ASSERT(friends != NULL);
    friendslist = new QTableView(this);
    friendslist->setModel(friends);
    friendslist->setSelectionBehavior(QTableView::SelectRows);
    //friendslist->setStretchLastColumn(true);
    friendslist->setColumnWidth(COL_NAME, 150);
    friendslist->setColumnWidth(COL_EID, 250);
    friendslist->setColumnWidth(COL_TALK, 75);
    friendslist->verticalHeader()->hide();
    connect(friendslist, SIGNAL(clicked(const QModelIndex&)),
        this, SLOT(friendsClicked(const QModelIndex&)));
    connect(friendslist->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(updateMenus()));
    setCentralWidget(friendslist);

    // Create a "Friends" toolbar providing friends list controls
    // XX need icons
    QToolBar *friendsbar = new QToolBar(tr("Friends"), this);
    taMessage = friendsbar->addAction(tr("Message"),
                    this, SLOT(openChat()));
    taTalk = friendsbar->addAction(tr("Talk"), this, SLOT(startTalk()));
    friendsbar->addSeparator();
    friendsbar->addAction(tr("My Profile"), this, SLOT(openProfile()));
    friendsbar->addAction(tr("Find Friends"), this, SLOT(openSearch()));
    taRename = friendsbar->addAction(tr("Rename"),
                    this, SLOT(renameFriend()));
    taDelete = friendsbar->addAction(tr("Delete"),
                    this, SLOT(deleteFriend()));
    addToolBar(friendsbar);

    // Create a "Friends" menu providing the same basic controls
    QMenu *friendsmenu = new QMenu(tr("Friends"), this);
    maMessage = friendsmenu->addAction(tr("&Message"),
                this, SLOT(openChat()),
                tr("Ctrl+M", "Friends|Message"));
    maTalk = friendsmenu->addAction(tr("&Talk"), this, SLOT(startTalk()),   
                tr("Ctrl+T", "Friends|Talk"));
    friendsmenu->addSeparator();
    friendsmenu->addAction(tr("&Find Friends"), this, SLOT(openSearch()),
                tr("Ctrl+F", "Friends|Find"));
    maRename = friendsmenu->addAction(tr("&Rename Friend"),
                this, SLOT(renameFriend()),
                tr("Ctrl+R", "Friends|Rename"));
    maDelete = friendsmenu->addAction(tr("&Delete Friend"),
                this, SLOT(deleteFriend()),
                tr("Ctrl+Delete", "Friends|Delete"));
    friendsmenu->addSeparator();
    friendsmenu->addAction(tr("&Settings..."), this,
                SLOT(openSettings()),
                tr("Ctrl+S", "Friends|Settings"));
    friendsmenu->addSeparator();
    friendsmenu->addAction(tr("E&xit"), this, SLOT(exitApp()),
                tr("Ctrl+X", "Friends|Exit"));
    menuBar()->addMenu(friendsmenu);

    // Create a "Window" menu
    QMenu *windowmenu = new QMenu(tr("Window"), this);
    windowmenu->addAction(tr("Friends"), this, SLOT(openFriends()));
    windowmenu->addAction(tr("Search"), this, SLOT(openSearch()));
    windowmenu->addAction(tr("Download"), this, SLOT(openDownload()));
    windowmenu->addAction(tr("Settings"), this, SLOT(openSettings()));
    windowmenu->addSeparator();
    windowmenu->addAction(tr("Log window"), this, SLOT(openLogWindow()));
    windowmenu->addSeparator();
    windowmenu->addAction(tr("Go to files"), this, SLOT(gotoFiles()));
    menuBar()->addMenu(windowmenu);

    // Create a "Help" menu
    QMenu *helpmenu = new QMenu(tr("Help"), this);
    helpmenu->addAction(tr("Netsteria &Help"), this, SLOT(openHelp()),
                tr("Ctrl+H", "Help|Help"));
    helpmenu->addAction(tr("Netsteria Home Page"), this, SLOT(openWeb()));
    helpmenu->addAction(tr("About Netsteria..."), this, SLOT(openAbout()));
    //menuBar()->addMenu(helpmenu);

    // Watch for state changes that require updating the menus.
    connect(friendslist->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(updateMenus()));
    connect(talksrv, SIGNAL(statusChanged(const QByteArray&)),
        this, SLOT(updateMenus()));

    // Retrieve the main window settings
    settings->beginGroup("MainWindow");
    move(settings->value("pos", QPoint(100, 100)).toPoint());
    resize(settings->value("size", QSize(800, 300)).toSize());
    settings->endGroup();

    updateMenus();

    // Add a Netsteria icon to the system tray, if possible
    QSystemTrayIcon *trayicon = new QSystemTrayIcon(appicon, this);
    connect(trayicon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this, SLOT(trayActivate(QSystemTrayIcon::ActivationReason)));
    trayicon->show();

#ifdef MAC_OS_X_VERSION_10_6
    MacSupport *m = new MacSupport();
    connect(m, SIGNAL(dockClicked()), this, SLOT(show()));
//    connect(m, SIGNAL(dockClicked()), m, SLOT(requestAttention()));
//    connect(m, SIGNAL(dockClicked()), m, SLOT(setDockBadge("ololo"));
#endif
}

MainWindow::~MainWindow()
{
    qDebug("MainWindow destructor");

    // Save the main window settings
    settings->beginGroup("MainWindow");
    settings->setValue("pos", pos());
    settings->setValue("size", size());
    settings->endGroup();
}

void MainWindow::trayActivate(QSystemTrayIcon::ActivationReason reason)
{
    qDebug("MainWindow::trayActivate - reason %d", (int)reason);
    openFriends();
}

int MainWindow::selectedFriend()
{
    return friendslist->selectionModel()->currentIndex().row();
}

void MainWindow::updateMenus()
{
    int row = selectedFriend();
    bool sel = isActiveWindow() && row >= 0;
    QByteArray id = sel ? friends->id(row) : QByteArray();

    maMessage->setEnabled(sel);
    taMessage->setEnabled(sel);
    maTalk->setEnabled(sel && talksrv->outConnected(id));
    taTalk->setEnabled(sel && talksrv->outConnected(id));
    maRename->setEnabled(sel);
    taRename->setEnabled(sel);
    maDelete->setEnabled(sel);
    taDelete->setEnabled(sel);
}

bool MainWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        updateMenus();
        break;
    default:
        break;
    }

    return QMainWindow::event(event);
}

void MainWindow::friendsClicked(const QModelIndex &index)
{
    int row = index.row();
    if (row < 0 || row >= friends->count())
        return;
    QByteArray hostid = friends->id(row);

    int col = index.column();
    if (col == COL_TALK)
        talksrv->toggleTalkEnabled(hostid);
}

void MainWindow::openFriends()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::openSearch()
{
    if (!searcher)
        searcher = new SearchDialog(this);
    searcher->present();
}

void MainWindow::openDownload()
{
    SaveDialog::present();
}

void MainWindow::openLogWindow()
{
    LogWindow::get().show();
}

void MainWindow::gotoFiles()
{
    QString dir = QString("file://") + shareDir.path();
    QDesktopServices::openUrl(QUrl(dir, QUrl::TolerantMode));
}

void MainWindow::openChat()
{
    int row = friendslist->selectionModel()->currentIndex().row();
    if (row < 0 || row >= friends->count())
        return;

    ChatDialog::open(friends->id(row), friends->name(row));
}

void MainWindow::startTalk()
{
    int row = selectedFriend();
    if (row <= 0 || row >= friends->count())
        return;

    talksrv->toggleTalkEnabled(friends->id(row));
}

void MainWindow::openSettings()
{
    SettingsDialog::openSettings();
}

void MainWindow::openProfile()
{
    SettingsDialog::openProfile();
}

void MainWindow::openHelp()
{
}

void MainWindow::openWeb()
{
}

void MainWindow::openAbout()
{
    QMessageBox *mbox = new QMessageBox(tr("About MettaNode"),
                tr("Based on Netsteria version 0.01\n"
                   "by Bryan Ford, © 2006"),
                QMessageBox::Information,
                QMessageBox::Ok, QMessageBox::NoButton,
                QMessageBox::NoButton, this);
    mbox->show();
    //mbox->setAttribute(Qt::WA_DeleteOnClose, true);
}

void MainWindow::renameFriend()
{
    int row = friendslist->selectionModel()->currentIndex().row();
    if (row < 0 || row >= friends->count())
        return;

    friendslist->edit(friends->index(row, 0));
}

void MainWindow::deleteFriend()
{
    int row = friendslist->selectionModel()->currentIndex().row();
    if (row < 0 || row >= friends->count())
        return;
    QString name = friends->name(row);
    QByteArray id = friends->id(row);

    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete '%0' from your contacts?").arg(name),
            QMessageBox::Yes | QMessageBox::Default,
            QMessageBox::No | QMessageBox::Escape)
            != QMessageBox::Yes)
        return;

    qDebug() << "Removing" << name;
    friends->remove(id);
}

void MainWindow::addPeer(const QByteArray &id, QString name, bool edit)
{
    int row = friends->insert(id, name);

    if (edit) {
        qDebug() << "Edit friend" << row;
        //friendslist->setCurrentIndex(peersmodel->index(row, 0));
        friendslist->edit(friends->index(row, 0));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug("MainWindow close");

    // If there's no system tray, exit immediately
    // because the user would have no way to get us open again.
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        exitApp();

    // Hide our main window but otherwise ignore the event,
    // waiting quietly in the background until we're re-activated.
    event->ignore();
    hide();
}

void MainWindow::exitApp()
{
    qDebug("MainWindow exit");

    QApplication::exit(0);
}

//=====================================================================================================================
// Helper functions.
//=====================================================================================================================

Puncher::Puncher(int p)
    : QObject()
    , port(p)
{
    qDebug() << "Puncher waiting for victims";
}

void Puncher::routerFound(UPnPRouter* r)
{
    qDebug() << "Router detected, punching a hole.";
    Port p(port, Port::UDP);
    connect(r, SIGNAL(portForwarded(bool)),
        this, SLOT(portForwarded(bool)));
    r->forward(p, /*leaseDuration:*/ 3600, /*extPort:*/ 0);
}

void Puncher::portForwarded(bool success)
{
    qDebug() << __PRETTY_FUNCTION__ << success;
    finished_punch = 1;
}

static void regclient(const QString &hostname)
{
    RegClient *regcli = new RegClient(ssthost);
    regcli->setInfo(myreginfo);
    regcli->registerAt(hostname);
    regclients.append(regcli);
}

static void usage()
{
    qCritical("Usage: netsteria [-d] [<configdir>]\n"
        "  -d              Enable debugging output\n"
        "  <configdir>     Optional config state directory\n");
    exit(1);
}

//=====================================================================================================================
// Main entry point.
//=====================================================================================================================

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("Exquance.com");
    app.setOrganizationDomain("exquance.com"); // for OSX
    app.setApplicationName("MettaNode");
    app.setQuitOnLastWindowClosed(false);

    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {
        case 'd':
            spewdebug = true;
            break;
        default:
            usage();
        }
        argc--, argv++;
    }

    if (argc > 1) {
        if (argc > 2 || argv[1][0] == '-') {
            usage();
        }
        QDir::current().mkdir(argv[1]);
        appdir.setPath(argv[1]);

        settings = new QSettings(appdir.path() + "/config",
                                QSettings::IniFormat);
        if (settings->status() != QSettings::NoError) {
                qFatal("Can't open config file in dir '%s'\n",
                        argv[1]);
        }
    } else {
        QDir homedir = QDir::home();
        QString homedirpath = homedir.path();

        QString appdirname = ".netsteria";
        homedir.mkdir(appdirname);
        appdir.setPath(homedirpath + "/" + appdirname);

        settings = new QSettings();
    }

    // Send debugging output to a log file
    QString logname(appdir.path() + "/log");
    QString logbakname(appdir.path() + "/log.bak");
    QFile::remove(logbakname);
    QFile::rename(logname, logbakname);
    logfile.setFileName(logname);
    if (!logfile.open(QFile::WriteOnly | QFile::Truncate))
        qWarning("Can't open log file '%s'",
            logname.toLocal8Bit().data());
    else
        qInstallMsgHandler(myMsgHandler);
    qDebug() << "Netsteria starting";

#if 0
//  openDefaultLog();

//  assert(argc == 3);  // XXX
//  mydev.setUserName(QString::fromAscii(argv[1]));
//  mydev.setDevName(QString::fromAscii(argv[2]));
    mydev.setUserName(QString::fromAscii("Bob"));
    mydev.setDevName(QString::fromAscii("phone"));

    keyinit();
    mydev.setEID(mykey->eid);
#endif
/*
    Puncher* p = new Puncher(NETSTERIA_DEFAULT_PORT);
    bt::UPnPMCastSocket* sock = new bt::UPnPMCastSocket(true);
    QObject::connect(sock, SIGNAL(discovered(UPnPRouter*)),
        p, SLOT(routerFound(UPnPRouter*)));
    sock->discover();

    while (!finished_punch)
        qApp->processEvents();
*/
    // Initialize the Structured Stream Transport
    ssthost = new Host(settings, NETSTERIA_DEFAULT_PORT);

    // Initialize the settings system, read user profile
    SettingsDialog::init();

    // XXX user info dialog
    myreginfo.setHostName(profile->hostName());
    myreginfo.setOwnerName(profile->ownerName());
    myreginfo.setCity(profile->city());
    myreginfo.setRegion(profile->region());
    myreginfo.setCountry(profile->country());
    myreginfo.setEndpoints(ssthost->activeLocalEndpoints());
    qDebug() << "local endpoints" << myreginfo.endpoints().size();

    // if (!settings->contains("regservers"))
    {
        QStringList rs;
        rs << "section4.madfire.net";
        settings->setValue("regservers", rs);
    }

    // XXX allow user-modifiable set of regservers
    foreach (QString server, settings->value("regservers").toStringList())
    {
        regclient(server);
    }

    // Load and initialize our friends table
    friends = new PeerTable(NCOLS);
    friends->setHeaderData(COL_ONLINE, Qt::Horizontal,
                QObject::tr("Presence"), Qt::DisplayRole);
    friends->setHeaderData(COL_FILES, Qt::Horizontal,
                QObject::tr("Files"), Qt::DisplayRole);
    friends->setHeaderData(COL_TALK, Qt::Horizontal,
                QObject::tr("Talk"), Qt::DisplayRole);
    friends->setHeaderData(COL_LISTEN, Qt::Horizontal,
                QObject::tr("Listen"), Qt::DisplayRole);
    friends->useSettings(settings, "Friends");

    PeerService* s = new PeerService("Presence", QObject::tr("Presence updates"),
                                     "NstPresence", QObject::tr("Netsteria presence protocol"));
    s->setPeerTable(friends);
    s->setStatusColumn(COL_ONLINE);

    // Initialize our chunk sharing service
    ChunkShare::instance()->setPeerTable(friends);
    ChunkShare::instance()->setStatusColumn(COL_FILES);

    // Share default directory
    appdir.mkdir("Files");
    shareDir = appdir.path() + "/Files";
    qDebug() << "Would share files from " << shareDir.path();
    // or read from Settings...
    FileSync *syncwatch = new FileSync;
    Share* share = new Share(0, shareDir.path());

    talksrv = new VoiceService();
    talksrv->setPeerTable(friends);
    talksrv->setTalkColumn(COL_TALK);
    talksrv->setListenColumn(COL_LISTEN);

    mainwin = new MainWindow;

    // Start our chat server to accept chat connections
    new ChatServer(mainwin);

    // Re-start incomplete downloads
    SaveDialog::init();

    mainwin->show();
    int r = app.exec();

    delete syncwatch;
    return r;
}
