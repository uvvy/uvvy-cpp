//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QHostInfo>
#include <thread>
#include "PeerPicker.h"
#include "PeerTableModel.h"
#include "settings_provider.h"
#include "host.h"
#include "client_profile.h"
#include "traverse_nat.h"
#include "audio_service.h"

using namespace std;
using namespace ssu;

class PeerPicker::Private
{
public:
    shared_ptr<settings_provider> settings;
    shared_ptr<host> host;
    shared_ptr<upnp::UpnpIgdClient> nat;
    std::thread runner;
    std::thread gone_natty;
    audio_service audioclient_;

    Private()
        : settings(settings_provider::instance())
        , host(host::create(settings))
        , runner([this] { host->run_io_service(); })
        , gone_natty([this] { nat = traverse_nat(host); })
        , audioclient_(host)
    {
        audioclient_.listen_incoming_session();
    }
};

PeerPicker::PeerPicker(QWidget *parent)
    : QMainWindow(parent)
    , m_pimpl(make_shared<Private>())
{
    setupUi(this); // this sets up GUI

    peersTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    peersTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    PeerTableModel* peers = new PeerTableModel(m_pimpl->host, m_pimpl->settings, this);
    peersTableView->setModel(peers);

    m_pimpl->audioclient_.on_session_started.connect([this] {
        selectedPeerText->appendPlainText("Call started");
        buttonCall->setText("Hang up");
    });

    connect(actionCall, SIGNAL(triggered()), this, SLOT(call()));
    connect(actionChat, SIGNAL(triggered()), this, SLOT(chat()));
    connect(actionAdd_to_favorites, SIGNAL(triggered()), this, SLOT(addToFavorites()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
}

void PeerPicker::addToFavorites()
{
    QMessageBox::information(this, "Not available", "Not implemented");
}

void PeerPicker::call()
{
    //get the text of the selected item
    const QModelIndex index = peersTableView->selectionModel()->currentIndex();
    const QModelIndex index2 = index.model()->index(index.row(), 1);
    QString selectedText = index2.data(Qt::DisplayRole).toString();


    m_pimpl->audioclient_.establish_outgoing_session(
        string(selectedText.toUtf8().constData()), vector<string>());
}

void PeerPicker::chat()
{
    QMessageBox::information(this, "Not available", "Not implemented");
}

void PeerPicker::quit()
{
    // @todo Check if need to save
    qDebug() << "Quitting";
    QApplication::quit();
}
