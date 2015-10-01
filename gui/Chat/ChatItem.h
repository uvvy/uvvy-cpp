#pragma once

class ChatItem
{
public:
	ChatItem()
	{

	}

	virtual ~ChatItem()
	{

	}

	virtual QVariant data(int column, int role) const = 0;

	virtual Qt::ItemFlags flags(int /* column */, Qt::ItemFlags flags) const
	{
		return flags;
	}

	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, int column) const = 0;

	virtual QSize sizeHint(const QStyleOptionViewItem &option, int column) const = 0;
};
