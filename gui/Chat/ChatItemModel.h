#pragma once

#include <QAbstractTableModel>

class ChatItem;

class ChatItemModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ChatItemModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int addItem(ChatItem* item);
    std::shared_ptr<ChatItem> item(int index) const;

    const QString& searchText() const;
    void setSearchText(const QString& searchText);
    void clearSearchText();

    const QRegExp& regExp() const;

private:
    QVector<std::shared_ptr<ChatItem>> _items;
    QString _searchText;
    QRegExp _regExp;
};
