#include <QHostInfo>
#include "profile_editor.h"
#include "settings_provider.h"
#include "host.h"
#include "client_profile.h"

using namespace std;
using namespace ssu;

class MainWindow::Private
{
public:
    shared_ptr<settings_provider> settings;
    shared_ptr<host> host;

    Private()
        : settings(settings_provider::instance())
        , host(host::create(settings.get()))
    {}
};

// Simple client_profile editor.
// Open settings_provider with default settings (or a specified settings file),
// display current data and allow editing profile fields using Qt4 gui.
// Save profile back.
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_pimpl(make_shared<Private>())
{
    setupUi(this); // this sets up GUI

    load();

    connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
    connect(actionGenerate_new_host_ID, SIGNAL(triggered()), this, SLOT(generateNewEid()));
}

void MainWindow::load()
{
    qDebug() << "Loading settings";
    auto s_port = m_pimpl->settings->get("port");
    if (!s_port.empty()) {
        portSpinBox->setValue(boost::any_cast<long long>(s_port));
    } else {
        portSpinBox->setValue(stream_protocol::default_port);
    }

    auto s_client = m_pimpl->settings->get("profile");
    if (!s_client.empty()) {
        uia::routing::client_profile client(boost::any_cast<vector<char>>(s_client));
        firstNameLineEdit->setText(client.owner_firstname().c_str());
        lastNameLineEdit->setText(client.owner_lastname().c_str());
        nicknameLineEdit->setText(client.owner_nickname().c_str());
        cityLineEdit->setText(client.city().c_str());
        regionLineEdit->setText(client.region().c_str());
        countryLineEdit->setText(client.country().c_str());
    }

    boost::any s_rs = m_pimpl->settings->get("regservers");
    if (!s_rs.empty())
    {
        byte_array rs_ba(boost::any_cast<vector<char>>(s_rs));
        byte_array_iwrap<flurry::iarchive> read(rs_ba);
        vector<string> regservers;
        read.archive() >> regservers;
        for (auto server : regservers)
        {
            routingServersTextEdit->appendPlainText(server.c_str());
        }
    }

    hostEIDLineEdit->setText(ssu::peer_id(m_pimpl->host->host_identity().id().id()).to_string().c_str());
}

void MainWindow::save()
{
    qDebug() << "Saving settings";
    m_pimpl->settings->set("port", (long long)portSpinBox->value());
    uia::routing::client_profile client;
    client.set_host_name(QHostInfo::localHostName().toUtf8().constData());
    client.set_owner_firstname(firstNameLineEdit->text().toUtf8().constData());
    client.set_owner_lastname(lastNameLineEdit->text().toUtf8().constData());
    client.set_owner_nickname(nicknameLineEdit->text().toUtf8().constData());
    client.set_city(cityLineEdit->text().toUtf8().constData());
    client.set_region(regionLineEdit->text().toUtf8().constData());
    client.set_country(countryLineEdit->text().toUtf8().constData());
    m_pimpl->settings->set("profile", client.enflurry());

    QStringList servers = routingServersTextEdit->toPlainText().split(QRegExp(" |\r|\n"), QString::SkipEmptyParts);
    vector<string> regservers;
    Q_FOREACH(QString s, servers) {
        regservers.emplace_back(s.toUtf8().constData());
    }
    byte_array rs_ba;
    {
        byte_array_owrap<flurry::oarchive> write(rs_ba);
        write.archive() << regservers;
    }
    m_pimpl->settings->set("regservers", rs_ba);

    m_pimpl->settings->sync();
}

void MainWindow::quit()
{
    // @todo Check if need to save
    qDebug() << "Quitting";
    QApplication::quit();
}

void MainWindow::generateNewEid()
{
    qDebug() << "Generating new host EID";
}
