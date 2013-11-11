#include <QHostInfo>
#include "profile_editor.h"
#include "settings_provider.h"
#include "identity.h"
#include "client_profile.h"

using namespace std;
using namespace ssu;

class MainWindow::Private
{
public:
    shared_ptr<settings_provider> settings;
    identity ident;

    Private()
        : settings(settings_provider::instance())
        , ident()
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

    byte_array id = m_pimpl->settings->get_byte_array("id");
    byte_array key = m_pimpl->settings->get_byte_array("key");

    if (!id.is_empty() and !key.is_empty())
    {
        m_pimpl->ident.set_id(id);
        if (m_pimpl->ident.set_key(key) && m_pimpl->ident.has_private_key())
        {
            hostEIDLineEdit->setText(peer_id(m_pimpl->ident.id().id()).to_string().c_str());
            setStatus("Profile loaded");
            return;
        }
    }

    // Generate a new key pair
    m_pimpl->ident = identity::generate();

    hostEIDLineEdit->setText(peer_id(m_pimpl->ident.id().id()).to_string().c_str());
    setStatus("New host EID generated");
}

void MainWindow::setStatus(QString const& text)
{
    statusLabel->setText(text);
    QTimer::singleShot(5000/*msec*/, this, SLOT(clearStatus()));
}

void MainWindow::clearStatus()
{
    statusLabel->clear();
}

void MainWindow::save()
{
    qDebug() << "Saving settings";
    m_pimpl->settings->set("id", m_pimpl->ident.id().id().as_vector());
    m_pimpl->settings->set("key", m_pimpl->ident.private_key().as_vector());

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
    setStatus("Profile saved");
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

    QMessageBox msgBox;
    msgBox.setText("Generate new host EID");
    msgBox.setInformativeText("Generating new EID will change your private key. You will lose your old private key and host EID. Do you want to create a new private key?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Cancel)
        return;

    m_pimpl->ident = identity::generate();
    hostEIDLineEdit->setText(peer_id(m_pimpl->ident.id().id()).to_string().c_str());
    setStatus("New EID generated");
}
