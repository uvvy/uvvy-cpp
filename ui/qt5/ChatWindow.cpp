#include "ChatWindow.h"
#include "ChatModel.h"
#include <QtQml/QQmlContext>
#include <QDebug>

ChatWindow::ChatWindow(ChatModel* model)
    : QmlBasedWindow("qrc:/quick/ChatWindow.qml")
{
    context_->setContextProperty("chatModel", model);
    QObject::connect(window_, SIGNAL(sendMessage(QString)),
                     this, SLOT(sendMessage(QString)));
}

void ChatWindow::sendMessage(QString const& text)
{
    qDebug() << "Sending chat message " << text;
}
