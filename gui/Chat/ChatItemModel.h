#pragma once

class ChatItem;

class ChatItemModel: public QAbstractTableModel
{
	Q_OBJECT

public:
	ChatItemModel();

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	int columnCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	int addItem(ChatItem *item);

private:
	QVector<QSharedPointer<ChatItem>> _items;
	QBrush _timeBrush;
};
