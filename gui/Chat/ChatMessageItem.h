#pragma once

#include "Chat/ChatItem.h"
#include <QTextEdit>
#include <QTime>

class ChatMessageItem : public ChatItem
{
public:
    ChatMessageItem()
    {
        _textEdit.setStyleSheet("QTextEdit { margin-left: -2px; margin-top: -2px }");
    }

    ChatMessageItem(const QString& nickName,
                    const QString& text,
                    const QTime& time,
                    const QColor& nickColor)
        : _nickName(nickName)
        , _text(text)
        , _time(time.toString())
        , _nickColor(nickColor)
    {
        ChatMessageItem();
    }

    ItemType type() const override;

    QVariant data(int column, int role) const override;

    Qt::ItemFlags flags(int column, Qt::ItemFlags flags) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, int column) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, int column) const override;

    int version() const override;

    void read(QDataStream& ds, int version) override;

    void write(QDataStream& ds) override;

    void readVersion1(QDataStream& ds);

    void writeVersion1(QDataStream& ds);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option) const override;

private:
    QString _nickName;
    QString _text;
    QString _time;
    QColor _nickColor;
    static QColor _textColor;
    static QColor _timeColor;
    mutable QTextEdit _textEdit;
};
