#include <QMessageBox>
#include <QCloseEvent>
#include <QTableView>
#include <QHeaderView>
#include <QApplication>
#include "MainWindow.h"
#include "PeerTableModel.h"
#include "macsupport.h"

// Columns in PeerTable
#define NCOLS       6
// default ones
#define COL_NAME    0
#define COL_EID     1
// custom
#define COL_ONLINE  2
#define COL_FILES   3
#define COL_TALK    4
#define COL_LISTEN  5

MainWindow::MainWindow(PeerTableModel* model)
{
    QIcon appicon(":/img/mettanode.png");

    setWindowTitle(tr("MettaNode"));
    setWindowIcon(appicon);

    // Create a ListView onto our friends list, as the central widget
    Q_ASSERT(model);
    peerlist = new QTableView(this);
    peerlist->setModel(model);
    peerlist->setSelectionBehavior(QTableView::SelectRows);
    //peerlist->setStretchLastColumn(true);
    peerlist->setColumnWidth(COL_NAME, 150);
    peerlist->setColumnWidth(COL_EID, 250);
    peerlist->setColumnWidth(COL_TALK, 75);
    peerlist->verticalHeader()->hide();
    connect(peerlist, SIGNAL(clicked(const QModelIndex&)),
        this, SLOT(friendsClicked(const QModelIndex&)));
    connect(peerlist->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(updateMenus()));
    setCentralWidget(peerlist);

    // Create a "Friends" toolbar providing friends list controls
    // XX need icons
    // QToolBar *friendsbar = new QToolBar(tr("Friends"), this);
    // taMessage = friendsbar->addAction(tr("Message"),
    //                 this, SLOT(openChat()));
    // taTalk = friendsbar->addAction(tr("Talk"), this, SLOT(startTalk()));
    // friendsbar->addSeparator();
    // friendsbar->addAction(tr("My Profile"), this, SLOT(openProfile()));
    // friendsbar->addAction(tr("Find Friends"), this, SLOT(openSearch()));
    // taRename = friendsbar->addAction(tr("Rename"),
    //                 this, SLOT(renameFriend()));
    // taDelete = friendsbar->addAction(tr("Delete"),
    //                 this, SLOT(deleteFriend()));
    // addToolBar(friendsbar);

    // Create a "Friends" menu providing the same basic controls
    // QMenu *friendsmenu = new QMenu(tr("Friends"), this);
    // maMessage = friendsmenu->addAction(tr("&Message"),
    //             this, SLOT(openChat()),
    //             tr("Ctrl+M", "Friends|Message"));
    // maTalk = friendsmenu->addAction(tr("&Talk"), this, SLOT(startTalk()),
    //             tr("Ctrl+T", "Friends|Talk"));
    // friendsmenu->addSeparator();
    // friendsmenu->addAction(tr("&Find Friends"), this, SLOT(openSearch()),
    //             tr("Ctrl+F", "Friends|Find"));
    // maRename = friendsmenu->addAction(tr("&Rename Friend"),
    //             this, SLOT(renameFriend()),
    //             tr("Ctrl+R", "Friends|Rename"));
    // maDelete = friendsmenu->addAction(tr("&Delete Friend"),
    //             this, SLOT(deleteFriend()),
    //             tr("Ctrl+Delete", "Friends|Delete"));
    // friendsmenu->addSeparator();
    // friendsmenu->addAction(tr("&Settings..."), this,
    //             SLOT(openSettings()),
    //             tr("Ctrl+S", "Friends|Settings"));
    // friendsmenu->addSeparator();
    // friendsmenu->addAction(tr("E&xit"), this, SLOT(exitApp()),
    //             tr("Ctrl+X", "Friends|Exit"));
    // menuBar()->addMenu(friendsmenu);

    // Create a "Window" menu
    // QMenu *windowmenu = new QMenu(tr("Window"), this);
    // windowmenu->addAction(tr("Friends"), this, SLOT(openFriends()));
    // windowmenu->addAction(tr("Search"), this, SLOT(openSearch()));
    // windowmenu->addAction(tr("Download"), this, SLOT(openDownload()));
    // windowmenu->addAction(tr("Settings"), this, SLOT(openSettings()));
    // windowmenu->addSeparator();
    // windowmenu->addAction(tr("Log window"), this, SLOT(openLogWindow()));
    // windowmenu->addSeparator();
    // windowmenu->addAction(tr("Go to files"), this, SLOT(gotoFiles()));
    // menuBar()->addMenu(windowmenu);

    // Create a "Help" menu
    // QMenu *helpmenu = new QMenu(tr("Help"), this);
    // helpmenu->addAction(tr("Netsteria &Help"), this, SLOT(openHelp()),
    //             tr("Ctrl+H", "Help|Help"));
    // helpmenu->addAction(tr("Netsteria Home Page"), this, SLOT(openWeb()));
    // helpmenu->addAction(tr("About Netsteria..."), this, SLOT(openAbout()));
    //menuBar()->addMenu(helpmenu);

    // Watch for state changes that require updating the menus.
    connect(peerlist->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(updateMenus()));
    // connect(talksrv, SIGNAL(statusChanged(const SST::PeerId&)),
    //     this, SLOT(updateMenus()));

    // Retrieve the main window settings
    // settings->beginGroup("MainWindow");
    // move(settings->value("pos", QPoint(100, 100)).toPoint());
    // resize(settings->value("size", QSize(800, 300)).toSize());
    // settings->endGroup();

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
    // settings->beginGroup("MainWindow");
    // settings->setValue("pos", pos());
    // settings->setValue("size", size());
    // settings->endGroup();
}

void MainWindow::trayActivate(QSystemTrayIcon::ActivationReason reason)
{
    qDebug("MainWindow::trayActivate - reason %d", (int)reason);
    // openFriends();
}

int MainWindow::selectedFriend()
{
    return peerlist->selectionModel()->currentIndex().row();
}

void MainWindow::updateMenus()
{
    // int row = selectedFriend();
    // bool sel = isActiveWindow() && row >= 0;
    // PeerId id = sel ? friends->id(row) : QByteArray();

    // maMessage->setEnabled(sel);
    // taMessage->setEnabled(sel);
    // maTalk->setEnabled(sel && talksrv->outConnected(id));
    // taTalk->setEnabled(sel && talksrv->outConnected(id));
    // maRename->setEnabled(sel);
    // taRename->setEnabled(sel);
    // maDelete->setEnabled(sel);
    // taDelete->setEnabled(sel);
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
    // int row = index.row();
    // if (row < 0 || row >= friends->count())
    //     return;
    // PeerId hostid = friends->id(row);

    // int col = index.column();
    // if (col == COL_TALK)
    //     talksrv->toggleTalkEnabled(hostid);
}

// void MainWindow::openFriends()
// {
//     show();
//     raise();
//     activateWindow();
// }

// void MainWindow::openSearch()
// {
//     if (!searcher)
//         searcher = new SearchDialog(this);
//     searcher->present();
// }

// void MainWindow::openDownload()
// {
//     SaveDialog::present();
// }

// void MainWindow::openLogWindow()
// {
//     // LogWindow::get().show();
// }

// void MainWindow::gotoFiles()
// {
//     QString dir = QString("file://") + shareDir.path();
//     QDesktopServices::openUrl(QUrl(dir, QUrl::TolerantMode));
// }

// void MainWindow::openChat()
// {
//     int row = peerlist->selectionModel()->currentIndex().row();
//     if (row < 0 || row >= friends->count())
//         return;

//     ChatDialog::open(friends->id(row), friends->name(row));
// }

// void MainWindow::startTalk()
// {
//     int row = selectedFriend();
//     if (row <= 0 || row >= friends->count())
//         return;

//     talksrv->toggleTalkEnabled(friends->id(row));
// }

// void MainWindow::openSettings()
// {
//     SettingsDialog::openSettings();
// }

// void MainWindow::openProfile()
// {
//     SettingsDialog::openProfile();
// }

void MainWindow::openAbout()
{
    QMessageBox *mbox = new QMessageBox(tr("About MettaNode"),
                tr("MettaNode 1.0\n"
                   "Decentralised internets client.\n"
                   "by Berkus © 2014"),
                QMessageBox::Information,
                QMessageBox::Ok, QMessageBox::NoButton,
                QMessageBox::NoButton, this);
    mbox->show();
    mbox->setAttribute(Qt::WA_DeleteOnClose, true);
}

// void MainWindow::renameFriend()
// {
//     int row = peerlist->selectionModel()->currentIndex().row();
//     if (row < 0 || row >= friends->count())
//         return;

//     peerlist->edit(friends->index(row, 0));
// }

// void MainWindow::deleteFriend()
// {
//     int row = peerlist->selectionModel()->currentIndex().row();
//     if (row < 0 || row >= friends->count())
//         return;
//     QString name = friends->name(row);
//     PeerId id = friends->id(row);

//     if (QMessageBox::question(this, tr("Confirm Delete"),
//             tr("Delete '%0' from your contacts?").arg(name),
//             QMessageBox::Yes | QMessageBox::Default,
//             QMessageBox::No | QMessageBox::Escape)
//             != QMessageBox::Yes)
//         return;

//     qDebug() << "Removing" << name;
//     friends->remove(id);
// }

// void MainWindow::addPeer(const QByteArray &id, QString name, bool edit)
// {
//     int row = friends->insert(id, name);

//     if (edit) {
//         qDebug() << "Edit friend" << row;
//         //peerlist->setCurrentIndex(peersmodel->index(row, 0));
//         peerlist->edit(friends->index(row, 0));
//     }
// }

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug("MainWindow close");

    // If there's no system tray, exit immediately
    // because the user would have no way to get us open again.
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        exitApp();
    }

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

