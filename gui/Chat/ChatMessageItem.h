#pragma once

#include "ChatItem.h"

class ChatMessageItem: public ChatItem
{
public:
	ChatMessageItem()
	{

	}

	ChatMessageItem(const QString &nickName, const QString &text, const QTime &time, const QColor &nickColor)
		: _nickName(nickName), _text(text), _time(time.toString()), _nickColor(nickColor)
	{

	}

	ItemType type() const override;

	QVariant data(int column, int role) const override;

	Qt::ItemFlags flags(int column, Qt::ItemFlags flags) const override;

	void paint(QPainter *painter, const QStyleOptionViewItem &option, int column) const override;

	QSize sizeHint(const QStyleOptionViewItem &option, int column) const override;

	int version() const override;

	void read(QDataStream &ds, int version) override;

	void write(QDataStream &ds) override;

	void readVersion1(QDataStream &ds);

	void writeVersion1(QDataStream &ds);

private:
	QString _nickName;
	QString _text;
	QString _time;
	QColor _nickColor;
	static QColor _textColor;
	static QColor _timeColor;
};
