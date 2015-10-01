#include "ChatItem.h"
#include "ChatItemModel.h"

ChatItemModel::ChatItemModel()
{

}

int ChatItemModel::rowCount(const QModelIndex &) const
{
	return _items.size();
}

int ChatItemModel::columnCount(const QModelIndex &) const
{
	return 3;
}

QVariant ChatItemModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
	{
		return 0;
	}

	int row = index.row();
	if(row < _items.size())
	{
		return _items[row]->data(index.column(), role);
	}
	else
	{
		return QVariant();
	}
}

int ChatItemModel::addItem(ChatItem *item)
{
	// TODO: Check how to do it in Qt 5
	beginResetModel();

	_items.push_back(std::shared_ptr<ChatItem>(item));

	endResetModel();

	return _items.size() - 1;
}

Qt::ItemFlags ChatItemModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
	{
		return 0;
	}

	int row = index.row();
	if(row < _items.size())
	{
		return _items[row]->flags(index.column(), QAbstractTableModel::flags(index));
	}
	else
	{
		return 0;
	}
}

std::shared_ptr<ChatItem> ChatItemModel::item(int index) const
{
	if(index < 0 || index >= _items.size())
	{
		return 0;
	}

	return _items.at(index);
}
