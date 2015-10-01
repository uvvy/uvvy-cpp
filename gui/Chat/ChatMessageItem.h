#pragma once

#include "ChatItem.h"

class ChatMessageItem: public ChatItem
{
public:
	ChatMessageItem(const QString &nickName, const QString &text, const QTime &time, const QColor &nickColor)
		: _nickName(nickName), _text(text), _time(time.toString()), _nickColor(nickColor)
	{

	}

	QVariant data(int column, int role) const override;

	Qt::ItemFlags flags(int column, Qt::ItemFlags flags) const override;

	void paint(QPainter *painter, const QStyleOptionViewItem &option, int column) const override;

	QSize sizeHint(const QStyleOptionViewItem &option, int column) const override;

private:
	QString _nickName;
	QString _text;
	QString _time;
	QColor _nickColor;
	static QColor _textColor;
	static QColor _timeColor;
};
