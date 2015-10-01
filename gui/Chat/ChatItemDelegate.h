#pragma once

class ChatItemModel;

class ChatItemDelegate: public QStyledItemDelegate
{
	Q_OBJECT

public:
	ChatItemDelegate(ChatItemModel *model);

	~ChatItemDelegate();

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;

	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
	ChatItemModel *_model = 0;

};
