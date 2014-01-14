//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QHostInfo>
#include <thread>
#include "PeerPicker.h"
#include "PeerTableModel.h"
#include "settings_provider.h"
#include "ssu/host.h"
#include "routing/client_profile.h"
#include "traverse_nat.h"
#include "voicebox/audio_service.h"

using namespace std;
using namespace ssu;

class PeerPicker::Private
{
public:
    shared_ptr<settings_provider> settings_;
    shared_ptr<host> host_;
    shared_ptr<upnp::UpnpIgdClient> nat_;
    std::thread runner_;
    voicebox::audio_service audioclient_;

    Private()
        : settings_(settings_provider::instance())
        , host_(host::create(settings_))
        , runner_([this] { host_->run_io_service(); })
        , audioclient_(host_)
    {
        nat_ = traverse_nat(host_);
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

    PeerTableModel* peers = new PeerTableModel(m_pimpl->host_, m_pimpl->settings_, this);
    peersTableView->setModel(peers);

    m_pimpl->audioclient_.on_session_started.connect([this] {
        // Less detailed logging while in real-time
        logger::set_verbosity(logger::verbosity::warnings);
        selectedPeerText->appendPlainText("Call started\n");
        buttonCall->setText("Hang up");
    });

    m_pimpl->audioclient_.on_session_finished.connect([this] {
        // More detailed logging while not in real-time
        logger::set_verbosity(logger::verbosity::debug);
        selectedPeerText->appendPlainText("Call finished\n");
        buttonCall->setText("Call");
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
    if (m_pimpl->audioclient_.is_active()) {
        m_pimpl->audioclient_.end_session();
    }
    else
    {
        //get the text of the selected item
        const QModelIndex index = peersTableView->selectionModel()->currentIndex();
        const QModelIndex index2 = index.model()->index(index.row(), 1);
        QString selectedText = index2.data(Qt::DisplayRole).toString();

        buttonCall->setText("Calling...");

        m_pimpl->audioclient_.establish_outgoing_session(
            string(selectedText.toUtf8().constData()), vector<string>());
    }
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
