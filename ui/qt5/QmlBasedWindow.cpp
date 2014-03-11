#include "QmlBasedWindow.h"
#include <QtQml/QQmlContext>
#include <QDebug>

QmlBasedWindow::QmlBasedWindow(QString qmlFile)
    : engine_()
    , component_(&engine_)
{
    context_ = new QQmlContext(engine_.rootContext());
    component_.loadUrl(QUrl(qmlFile));

    if (!component_.isReady() ) {
        qFatal("%s", component_.errorString().toUtf8().constData());
    }

    QObject *topLevel = component_.create(context_);
    window_ = qobject_cast<QQuickWindow*>(topLevel);

    QSurfaceFormat surfaceFormat = window_->requestedFormat();
    window_->setFormat(surfaceFormat);
}
