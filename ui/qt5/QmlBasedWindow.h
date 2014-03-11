#pragma once

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickWindow>

class QmlBasedWindow
{
protected:
    QQmlEngine engine_;
    QQmlComponent component_;
    QQuickWindow* window_{nullptr};
    QQmlContext *context_{nullptr};

public:
    QmlBasedWindow(QString qmlFile);
    inline void show() { window_->show(); }
};
