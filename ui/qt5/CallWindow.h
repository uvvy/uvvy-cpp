#pragma once

#include "QmlBasedWindow.h"

class CallWindow : public QObject, public QmlBasedWindow
{
    Q_OBJECT

public:
    CallWindow();
};
