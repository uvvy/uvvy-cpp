#include <QHostInfo>
#include <thread>
#include "PeerPicker.h"
#include "PeerInfoProvider.h"
#include "settings_provider.h"
#include "host.h"
#include "client_profile.h"
#include "traverse_nat.h"

using namespace std;
using namespace ssu;

class PeerPicker::Private
{
public:
    shared_ptr<settings_provider> settings;
    shared_ptr<host> host;
    // shared_ptr<upnp::UpnpIgdClient> nat;
    std::thread runner;

    Private()
        : settings(settings_provider::instance())
        , host(host::create(settings))
        //, nat(traverse_nat(stream_protocol::default_port)) // XXX take port from host
        , runner([this] { host->run_io_service(); })
    {}
};

// Simple client_profile editor.
// Open settings_provider with default settings (or a specified settings file),
// display current data and allow editing profile fields using Qt4 gui.
// Save profile back.
PeerPicker::PeerPicker(QWidget *parent)
    : QMainWindow(parent)
    , m_pimpl(make_shared<Private>())
{
    setupUi(this); // this sets up GUI

    PeerInfoProvider* pp = new PeerInfoProvider(m_pimpl->host, this);
    peersTableView->setModel(pp);

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
