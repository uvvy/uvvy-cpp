#pragma once

class ChatItemDelegate: public QStyledItemDelegate
{
	Q_OBJECT

public:
	ChatItemDelegate();
	~ChatItemDelegate();

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

private:
	QTextEdit *_textEdit = 0;

};
