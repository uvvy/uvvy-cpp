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

//
// Main application entrypoint
//
int main(int argc, char* argv[])
{
    // bool verbose_debug{true};
    // if (!verbose_debug) {
        // logger::set_verbosity(logger::verbosity::info);
    // }
    std::shared_ptr<ssu::host> host =
        ssu::host::create(settings_provider::instance());
    ContactModel* model = new ContactModel(host, settings_provider::instance());

    XcpApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QQmlEngine engine;
    QQmlComponent component(&engine);

    QQmlContext *context = new QQmlContext(engine.rootContext());
    context->setContextProperty("contactModel", model);

    QObject::connect(&engine, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));

    component.loadUrl(QUrl("qrc:/quick/MainWindow.qml"));

    if (!component.isReady() ) {
        qFatal("%s", component.errorString().toUtf8().constData());
    }

    QObject *topLevel = component.create(context);
    QQuickWindow *window = qobject_cast<QQuickWindow*>(topLevel);

    QSurfaceFormat surfaceFormat = window->requestedFormat();
    window->setFormat(surfaceFormat);
    window->show();

    return app.exec();
}
