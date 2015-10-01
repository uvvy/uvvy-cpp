#include "ChatTextEdit.h"

ChatTextEdit::ChatTextEdit(QWidget *parentWidget)
		: QPlainTextEdit(parentWidget)
{

}

ChatTextEdit::~ChatTextEdit()
{

}

void ChatTextEdit::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		emit returnPressed();
	}
	else
	{
		QPlainTextEdit::keyPressEvent(event);
	}
}
