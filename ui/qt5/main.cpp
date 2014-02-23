#include "ssu/host.h"
#include "arsenal/settings_provider.h"
#include "ContactModel.h"
#include "XcpApplication.h"
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QtCore/QUrl>
#include <QDebug>
#include <memory>
#include "traverse_nat.h"

using namespace std;
using namespace ssu;

/**
 * HostState embeds settings, NAT traversal and io_service runner for the SSU host.
 */
class HostState
{
    shared_ptr<settings_provider> settings_;
    shared_ptr<host> host_;
    shared_ptr<upnp::UpnpIgdClient> nat_;
    std::thread runner_; // Thread to run io_service (@todo Could be a thread pool)

public:
    HostState()
        : settings_(settings_provider::instance())
        , host_(host::create(settings_))
        , runner_([this] { host_->run_io_service(); })
    {
        nat_ = traverse_nat(host_);
    }

    inline shared_ptr<host> host() const { return host_; }
    inline shared_ptr<settings_provider> settings() const { return settings_; }
};

class MainWindow
{
    QQmlEngine engine_;
    QQmlComponent component_;
    QQuickWindow* window_;

public:
    MainWindow(ContactModel* model);
    inline void show() { window_->show(); }
};

MainWindow::MainWindow(ContactModel* model)
    : engine_()
    , component_(&engine_)
{
    QQmlContext *context = new QQmlContext(engine_.rootContext());
    context->setContextProperty("contactModel", model);

    QObject::connect(&engine_, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));

    component_.loadUrl(QUrl("qrc:/quick/MainWindow.qml"));

    if (!component_.isReady() ) {
        qFatal("%s", component_.errorString().toUtf8().constData());
    }

    QObject *topLevel = component_.create(context);
    window_ = qobject_cast<QQuickWindow*>(topLevel);

    QSurfaceFormat surfaceFormat = window_->requestedFormat();
    window_->setFormat(surfaceFormat);
}

//
// Main application entrypoint
//
int main(int argc, char* argv[])
{
    // bool verbose_debug{true};
    // if (!verbose_debug) {
        // logger::set_verbosity(logger::verbosity::info);
    // }
    HostState state;
    ContactModel* model = new ContactModel(state.host(), state.settings());

    XcpApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    MainWindow win(model);
    win.show();

    return app.exec();
}
