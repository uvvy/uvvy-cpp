#include "ContactModel.h"
#include "XcpApplication.h"
#include "HostState.h"
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QtCore/QUrl>
#include <QDebug>
#include <memory>

using namespace std;
using namespace ssu;

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
