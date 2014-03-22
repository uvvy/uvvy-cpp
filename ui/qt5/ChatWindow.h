#pragma once

#include "QmlBasedWindow.h"

class ChatModel;

class ChatWindow : public QObject, public QmlBasedWindow
{
    Q_OBJECT

public:
    ChatWindow(ChatModel* model);

public slots:
    void sendMessage(QString const& msg);
};
