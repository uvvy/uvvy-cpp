#include "ChatMessageTextEdit.h"

ChatMessageTextEdit::ChatMessageTextEdit(QWidget *parent)
		: QTextEdit(parent)
{
}

void ChatMessageTextEdit::showEvent(QShowEvent * /*event*/)
{
	if(!_showed)
	{
		_showed = true;

		QTextCursor cursor = cursorForPosition(mapFromGlobal(QCursor::pos()));
		setTextCursor(cursor);

		resize(size().width() + 4, size().height() + 4);
	}
}

bool ChatMessageTextEdit::eventFilter(QObject *object, QEvent *event)
{
	if(event->type() == QEvent::Wheel)
	{
		event->ignore();
		return true;
	}
	
	return QWidget::eventFilter(object, event);
}
