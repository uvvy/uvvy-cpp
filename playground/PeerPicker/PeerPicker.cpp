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
    // shared_ptr<upnp::UpnpIgdClient> nat;
    std::thread runner;
    audio_service audioclient_;

    Private()
        : settings(settings_provider::instance())
        , host(host::create(settings))
        //, nat(traverse_nat(stream_protocol::default_port)) // XXX take port from host
        , runner([this] { host->run_io_service(); })
        , audioclient_(host)
    {}
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

    load();

    connect(actionCall, SIGNAL(triggered()), this, SLOT(call()));
    connect(actionChat, SIGNAL(triggered()), this, SLOT(chat()));
    connect(actionAdd_to_favorites, SIGNAL(triggered()), this, SLOT(addToFavorites()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
}

void PeerPicker::load()
{
}

void PeerPicker::addToFavorites()
{
}

void PeerPicker::call()
{
    //get the text of the selected item
    const QModelIndex index = peersTableView->selectionModel()->currentIndex();
    QString selectedText = index.data(Qt::DisplayRole).toString();

    qDebug() << peersTableView->selectionModel()->selectedRows();
    qDebug() << selectedText;

    m_pimpl->audioclient_.establish_outgoing_session(
        string(selectedText.toUtf8().constData()), vector<string>());
}

void PeerPicker::chat()
{
}

void PeerPicker::quit()
{
    // @todo Check if need to save
    qDebug() << "Quitting";
    QApplication::quit();
}
