//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QHostInfo>
#include <QMessageBox>
#include <thread>
#include "PeerPicker.h"
#include "PeerTableModel.h"
#include "arsenal/settings_provider.h"
#include "ssu/host.h"
#include "routing/client_profile.h"
#include "voicebox/audio_service.h"

using namespace std;
using namespace ssu;

class PeerPicker::Private
{
public:
    voicebox::audio_service audioclient_;

    Private()
        , audioclient_(host_)
    {
        audioclient_.listen_incoming_session();
    }
};

PeerPicker::PeerPicker(QWidget *parent)
    : QMainWindow(parent)
    , m_pimpl(make_shared<Private>())
{
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
