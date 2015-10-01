#pragma once

class ChatItemModel;

class ChatItem
{
public:
	enum ItemType
	{
		CHAT_MESSAGE_ITEM,
		CHAT_CALL_ITEM
	};

	ChatItem()
	{

	}

	virtual ~ChatItem()
	{

	}

	virtual ItemType type() const = 0;

	virtual QVariant data(int column, int role) const = 0;

	virtual Qt::ItemFlags flags(int /* column */, Qt::ItemFlags flags) const
	{
		return flags;
	}

	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, int column) const = 0;

	virtual QSize sizeHint(const QStyleOptionViewItem &option, int column) const = 0;

	virtual int version() const = 0;

	virtual void read(QDataStream &ds, int version) = 0;

	virtual void write(QDataStream &ds) = 0;

	static ChatItem *createItem(ItemType itemType);

	virtual QWidget *createEditor(QWidget * /*parent*/, const QStyleOptionViewItem &/*option*/) const
	{
		return nullptr;
	}

	ChatItemModel *model() const
	{
		return _model;
	}

	void setModel(ChatItemModel *model)
	{
		_model = model;
	}

private:
	ChatItemModel *_model = 0;
};
