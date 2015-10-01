#include "Chat/ChatItem.h"
#include "Chat/ChatItemModel.h"

ChatItemModel::ChatItemModel()
{
    _regExp.setCaseSensitivity(Qt::CaseInsensitive);
}

int
ChatItemModel::rowCount(const QModelIndex&) const
{
    return _items.size();
}

int
ChatItemModel::columnCount(const QModelIndex&) const
{
    return 3;
}

QVariant
ChatItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return 0;
    }

    int row = index.row();
    if (row < _items.size()) {
        return _items[row]->data(index.column(), role);
    } else {
        return QVariant();
    }
}

int
ChatItemModel::addItem(ChatItem* item)
{
    item->setModel(this);

    // TODO: Check how to do it in Qt 5
    beginResetModel();

    _items.push_back(std::shared_ptr<ChatItem>(item));

    endResetModel();

    return _items.size() - 1;
}

Qt::ItemFlags
ChatItemModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return 0;
    }

    int row = index.row();
    if (row < _items.size()) {
        return _items[row]->flags(index.column(), QAbstractTableModel::flags(index));
    } else {
        return 0;
    }
}

std::shared_ptr<ChatItem>
ChatItemModel::item(int index) const
{
    if (index < 0 || index >= _items.size()) {
        return 0;
    }

    return _items.at(index);
}

const QString&
ChatItemModel::searchText() const
{
    return _searchText;
}

void
ChatItemModel::setSearchText(const QString& searchText)
{
    _searchText = searchText;

    _regExp.setPattern(_searchText);
}

void
ChatItemModel::clearSearchText()
{
    _searchText.clear();
}

const QRegExp&
ChatItemModel::regExp() const
{
    return _regExp;
}
