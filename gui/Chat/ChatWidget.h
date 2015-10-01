#pragma once

#include <QFile>

/*
 * Widget ChatWidget implements chat abilities.
 * Use setID() to set user id.
 * Signal sendMessage(id, text, time) appears when current user adds some message.
 * ChatWidget doesn't add anything to itself. Use addMessage and other methods
 * to add messages (or information about calls/missed calls) to chat widget
 * */

#include "Chat/ChatItem.h"

class Ui_Form;

class ChatItemModel;
class ChatItemDelegate;

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    ChatWidget(const QString& id, QWidget* parentWidget = 0);
    ~ChatWidget();

    static void setChatDirectory(const QString& chatDirectory);

    static const QString& chatDirectory();

    const QString& id() { return _id; }

    void addMessage(const QString& nickName,
                    const QString& message,
                    const QTime& time,
                    bool saveToHistory = true);

    void resizeEvent(QResizeEvent*) override;

    void showEvent(QShowEvent* event) override;

private:
    Ui_Form* ui;

public slots:
    // Internal slot for internal widgets. Don't use this
    void widgetSendMessage();

    // void saveEntry(ChatItem::ItemType type, const QString &nickName, const QString &text, const
    // QTime &time);

signals:
    void sendMessage(const QString& id, const QString& text, const QTime& time);

private:
    QString _id;
    QFile* _file = 0;
    QDataStream _chatFile;

    static QString _chatDirectory;

private:
    void loadChatHistory();
    void addItem(ChatItem* item, bool saveToHistory);

    ChatItemModel* _model       = 0;
    ChatItemDelegate* _delegate = 0;

    bool _showed = false;
};
