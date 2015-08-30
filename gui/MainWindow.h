#pragma once

#include "QmlWindow.h"

namespace gui {

class UserInfo;

class MainWindow : public QmlWindow
{
public:
    MainWindow(WindowManager* m, UserInfo* u);
    ~MainWindow();

private:
    QList<QObject*> contactList_;
};

}
