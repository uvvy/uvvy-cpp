#pragma once

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickWindow>

class ContactModel;

class MainWindow
{
    QQmlEngine engine_;
    QQmlComponent component_;
    QQuickWindow* window_;

public:
    MainWindow(ContactModel* model);
    inline void show() { window_->show(); }
};
