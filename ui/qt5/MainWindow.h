#pragma once

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickWindow>

class ContactModel;

class MainWindow : public QObject
{
    Q_OBJECT

    QQmlEngine engine_;
    QQmlComponent component_;
    QQuickWindow* window_;

public:
    MainWindow(ContactModel* model);
    inline void show() { window_->show(); }

public slots:
    void startCall(QString const& eid);
};
