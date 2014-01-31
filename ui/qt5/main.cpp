// #include "arsenal/logging.h"
// #include "ssu/host.h"
// #include "arsenal/settings_provider.h"
//#include "MainWindow.h"
// #include "PeerTableModel.h"

#include "XcpApplication.h"
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickWindow>
#include <QtCore/QUrl>
#include <QDebug>

//
// Main application entrypoint
//
int main(int argc, char* argv[])
{
    // bool verbose_debug{true};
    // if (!verbose_debug) {
        // logger::set_verbosity(logger::verbosity::info);
    // }
    // std::shared_ptr<ssu::host> host =
    //     ssu::host::create(settings_provider::instance());
    // PeerTableModel* model = new PeerTableModel(host);

    XcpApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    QQuickWindow::setDefaultAlphaBuffer(true);
    component.loadUrl(QUrl("qrc:/quick/MainWindow.qml"));
    if (component.isReady()) {
        component.create();
    }
    else {
        qWarning() << component.errorString();
    }
    return app.exec();
}
