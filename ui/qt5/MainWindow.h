#pragma once

#include "QmlBasedWindow.h"

class ContactModel;

class MainWindow : public QObject, public QmlBasedWindow
{
    Q_OBJECT

public:
    MainWindow(ContactModel* model);

public slots:
    void startCall(QString const& eid);
};
