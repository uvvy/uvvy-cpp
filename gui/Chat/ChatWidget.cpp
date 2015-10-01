#include "ChatWidget.h"
#include "ui_ChatWidget.h"
#include "ChatItem.h"
#include "ChatItemModel.h"
#include "ChatItemDelegate.h"

QString ChatWidget::_chatDirectory;

const int NICKNAME_COLUMN_WIDTH = 150;
const int TIME_COLUMN_WIDTH = 150;

ChatWidget::ChatWidget(const QString &id, QWidget *parentWidget)
		: QWidget(parentWidget)
		, _id(id)
		, _chatFile(_chatDirectory + QDir::separator() + _id)
{
	Q_ASSERT(!_chatDirectory.isEmpty() && "Please use setChatDirectory() before creating any ChatWidget");

	ui = new Ui_Form;

	ui->setupUi(this);

	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(widgetSendMessage()));
	connect(ui->chatEdit, SIGNAL(returnPressed()), this, SLOT(widgetSendMessage()));

	_model = new ChatItemModel();
	ui->tableView->setModel(_model);

	ui->tableView->setShowGrid(false);
	ui->tableView->verticalHeader()->setVisible(false);
	ui->tableView->horizontalHeader()->setVisible(false);

	ui->tableView->setWordWrap(true);
	// ui->tableView->setTextElideMode(Qt::ElideMiddle);
	
	ui->tableView->setColumnWidth(0, NICKNAME_COLUMN_WIDTH);
	ui->tableView->setColumnWidth(2, TIME_COLUMN_WIDTH);

	_delegate = new ChatItemDelegate();
	ui->tableView->setItemDelegate(_delegate);

	resizeEvent(0);

	if(!_chatFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		Q_ASSERT(!"Can't open chat file");
	}
	else
	{
		loadChatHistory();
	}

	ui->chatEdit->setFocus();
}

ChatWidget::~ChatWidget()
{
	if(_model != 0)
	{
		delete _model;
		_model = 0;
	}
}

void ChatWidget::widgetSendMessage()
{
	QString text = ui->chatEdit->toPlainText();

	if(!text.isEmpty())
	{
		// emit sendMessage(_id, text, QTime::currentTime());

		// Only for debug
		addMessage("Jordon", text, QTime::currentTime());
	}
}

void ChatWidget::addMessage(const QString &nickName, const QString &message, const QTime &time, bool saveToHistory)
{
	if(!message.isEmpty())
	{
		// TODO: Need to replace :wink: with smiles
		// QString text = message.toHtmlEscaped();

		// if(!text.isEmpty())
		{
			/*
			QString html = QString("<table width=\"100%\"><tr><td align=\"right\" width=\"175\"><font color=\"%1\">%2</font></td><td width=\"15\"></td><td width=\"100%\">%3</td><td align=\"right\"><font color=\"gray\">%4</font></td></tr></table>").arg("red").arg(nickName).arg(text).arg(time.toString());
			ui->textEdit->append(html);
			*/

			ChatItem *item = new ChatItem(nickName, message, time, QBrush(Qt::red));

			int row = _model->addItem(item);
			ui->tableView->resizeRowToContents(row);

			QScrollBar *scroll = ui->tableView->verticalScrollBar();
			scroll->setValue(scroll->maximum());

			ui->chatEdit->clear();
			ui->chatEdit->setFocus();

			if(saveToHistory)
			{
				saveEntry(CHAT_MESSAGE, nickName, message, time);
			}
		}
	}
}

void ChatWidget::setChatDirectory(const QString &chatDirectory)
{
	_chatDirectory = chatDirectory;

	QDir dir(_chatDirectory);

	if(!dir.exists())
	{
		dir.mkdir(_chatDirectory);
	}
}

const QString &ChatWidget::chatDirectory()
{
	return _chatDirectory;
}

void ChatWidget::saveEntry(EntryType type, const QString &nickName, const QString &text, const QTime &time)
{
	// type|nick|time|size
	QString buffer = QString("%1|%2|%3|%4\n%5\n").arg(type).arg(nickName).arg(time.toString()).arg(text.size()).arg(text);

	_chatFile.write(buffer.toUtf8());
}

void ChatWidget::loadChatHistory()
{
	if(_chatFile.isOpen())
	{
		while(!_chatFile.atEnd())
		{
			while(true)
			{
				char c;
				if(!_chatFile.getChar(&c))
				{
					return;
				}

				if(c == 0)
				{
					return;
				}

				if(c != '\r' && c != '\n')
				{
					_chatFile.ungetChar(c);
					break;
				}
			}

			QString buffer = _chatFile.readLine();
			QStringList list = buffer.split('|');

			if(list.size() != 4)
			{
				_chatFile.close();
				_chatFile.remove();
				_chatFile.open(QIODevice::ReadWrite | QIODevice::Text);
				Q_ASSERT(!"Chat file corrupted");
				return;
			}

			QStringList::const_iterator it = list.begin();

			bool ok = false;
			int type = (*it).toInt(&ok);

			if(!ok)
			{
				_chatFile.close();
				_chatFile.remove();
				_chatFile.open(QIODevice::ReadWrite | QIODevice::Text);
				Q_ASSERT(!"Chat file corrupted");
				return;
			}

			++it;
			QString nickName = *it;

			++it;
			QTime time = QTime::fromString(*it);

			++it;
			int size = (*it).toInt(&ok);
			if(!ok)
			{
				_chatFile.close();
				_chatFile.remove();
				_chatFile.open(QIODevice::ReadWrite | QIODevice::Text);
				Q_ASSERT(!"Chat file corrupted");
				return;
			}

			QString text = _chatFile.read(size);
			if(text.size() != size)
			{
				_chatFile.close();
				_chatFile.remove();
				_chatFile.open(QIODevice::ReadWrite | QIODevice::Text);
				Q_ASSERT(!"Chat file corrupted");
				return;
			}

			switch(type)
			{
				case CHAT_MESSAGE:
					addMessage(nickName, text, time, false);
					break;
			}
		}
	}
}

void ChatWidget::resizeEvent(QResizeEvent *event)
{
	ui->tableView->setColumnWidth(1, ui->tableView->width() - (NICKNAME_COLUMN_WIDTH + TIME_COLUMN_WIDTH + 60));
	ui->tableView->resizeRowsToContents();
}
