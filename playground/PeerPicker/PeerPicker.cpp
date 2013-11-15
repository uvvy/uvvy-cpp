#include <QHostInfo>
#include "PeerPicker.h"
#include "settings_provider.h"
#include "host.h"
#include "client_profile.h"

using namespace std;
using namespace ssu;

class PeerPicker::Private
{
public:
    shared_ptr<settings_provider> settings;
    shared_ptr<host> host;

    Private()
        : settings(settings_provider::instance())
        , host(host::create(settings))
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

    load();

    connect(actionCall, SIGNAL(triggered()), this, SLOT(call()));
    connect(actionChat, SIGNAL(triggered()), this, SLOT(chat()));
    connect(actionAdd_to_favorites, SIGNAL(triggered()), this, SLOT(addToFavorites()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
}

void PeerPicker::load()
{
    boost::any s_rs = m_pimpl->settings->get("regservers");
    if (!s_rs.empty())
    {
        byte_array rs_ba(boost::any_cast<vector<char>>(s_rs));
        byte_array_iwrap<flurry::iarchive> read(rs_ba);
        vector<string> regservers;
        read.archive() >> regservers;
        for (auto server : regservers)
        {
            // routingServersTextEdit->appendPlainText(server.c_str());
        }
    }
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
