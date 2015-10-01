#include "ChatItem.h"
#include "ChatItemModel.h"

ChatItemModel::ChatItemModel()
		: _timeBrush(Qt::gray)
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

QVariant ChatItemModel::data(const QModelIndex & index, int role) const
{
	int row = index.row();

	if(index.isValid() && row < _items.size())
	{
		switch(role)
		{
			case Qt::DisplayRole:
			case Qt::EditRole:
			{
				switch(index.column())
				{
					case 0:
						return _items[row]->nickName();
					case 1:
						return _items[row]->text();
					case 2:
						return _items[row]->time().toString();
					default:
						return QVariant();
				}
			}
			case Qt::ForegroundRole:
			{
				switch(index.column())
				{
					case 0:
						return _items[row]->nickColor();
					// case 1:
						// return _items[row]->text();
					case 2:
						return _timeBrush;
					default:
						return QVariant();
				}
			}
			case Qt::TextAlignmentRole:
			{
				switch(index.column())
				{
					case 0:
						return (int)(Qt::AlignLeft | Qt::AlignTop);
					case 1:
						return (int)(Qt::AlignLeft & Qt::AlignTop);
					case 2:
						return (int)(Qt::AlignRight | Qt::AlignTop);
					default:
						return QVariant();
				}
			}
			default:
			{
				return QVariant();
			}
		}
	}
	else
	{
		return QVariant();
	}
}

int ChatItemModel::addItem(ChatItem *item)
{
	// TODO: Check hot to do it in Qt 5
	beginResetModel();

	_items.push_back(QSharedPointer<ChatItem>(item));

	endResetModel();

	return _items.size() - 1;
}

Qt::ItemFlags ChatItemModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
	{
		return 0;
	}

	if(index.column() == 1)
	{
		return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
	}
	else
	{
		return QAbstractTableModel::flags(index);
	}
}
