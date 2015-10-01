#include "ChatItemDelegate.h"

ChatItemDelegate::ChatItemDelegate()
{
	_textEdit = new QTextEdit(0);
}

ChatItemDelegate::~ChatItemDelegate()
{
	if(_textEdit != 0)
	{
		delete _textEdit;
		_textEdit = 0;
	}
}

void ChatItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	painter->setPen(index.data(Qt::ForegroundRole).value<QBrush>().color());
	QRect rect = option.rect.adjusted(2, 2, 4, 4);

	painter->drawText(rect, index.data(Qt::TextAlignmentRole).toInt() | Qt::TextWordWrap, index.data().toString());
}

QSize ChatItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QRect rect = option.fontMetrics.boundingRect(option.rect, Qt::TextWordWrap, index.data().toString());

	rect.adjust(2, 2, 4, 4);
	return rect.size();
}
