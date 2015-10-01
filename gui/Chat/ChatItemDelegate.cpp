#include "ChatItem.h"
#include "ChatItemModel.h"
#include "ChatItemDelegate.h"

ChatItemDelegate::ChatItemDelegate(ChatItemModel *model)
		: _model(model)
{
}

ChatItemDelegate::~ChatItemDelegate()
{
}

void ChatItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if(_model != 0)
	{
		if(index.isValid())
		{
			std::shared_ptr<ChatItem> item = _model->item(index.row());
			
			if(item != 0)
			{
				item->paint(painter, option, index.column());
			}
		}
	}
}

QSize ChatItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if(_model != 0)
	{
		if(index.isValid())
		{
			std::shared_ptr<ChatItem> item = _model->item(index.row());
			
			if(item != 0)
			{
				return item->sizeHint(option, index.column());
			}
		}
	}

	return QSize();
}

QWidget *ChatItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if(_model != 0)
	{
		if(index.isValid())
		{
			std::shared_ptr<ChatItem> item = _model->item(index.row());
			
			if(item != 0)
			{
				return item->createEditor(parent, option);
			}
		}
	}

	return nullptr;
}
