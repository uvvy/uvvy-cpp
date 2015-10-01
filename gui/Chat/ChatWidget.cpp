#include "ChatWidget.h"
#include "ui_ChatWidget.h"
#include "ChatItemModel.h"
#include "ChatItemDelegate.h"
#include "ChatMessageItem.h"

QString ChatWidget::_chatDirectory;

const int NICKNAME_COLUMN_WIDTH = 150;
const int TIME_COLUMN_WIDTH = 150;

ChatWidget::ChatWidget(const QString &id, QWidget *parentWidget)
		: QWidget(parentWidget)
		, _id(id)
		, _file(new QFile(_chatDirectory + QDir::separator() + _id))
		, _chatFile(_file)
{
	Q_ASSERT(!_chatDirectory.isEmpty() && "Please use setChatDirectory() before creating any ChatWidget");

	if(_file != 0)
	{
		_file->open(QIODevice::ReadWrite);
	}

	ui = new Ui_Form;

	ui->setupUi(this);

	connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(widgetSendMessage()));
	connect(ui->chatEdit, SIGNAL(returnPressed()), this, SLOT(widgetSendMessage()));

	_model = new ChatItemModel();
	_delegate = new ChatItemDelegate(_model);

	ui->tableView->setItemDelegate(_delegate);
	ui->tableView->setModel(_model);

	ui->tableView->setShowGrid(false);
	ui->tableView->verticalHeader()->setVisible(false);
	ui->tableView->horizontalHeader()->setVisible(false);

	ui->tableView->setWordWrap(true);
	
	ui->tableView->setColumnWidth(0, NICKNAME_COLUMN_WIDTH);
	ui->tableView->setColumnWidth(2, TIME_COLUMN_WIDTH);

	loadChatHistory();
	resizeEvent(0);

	ui->chatEdit->setFocus();
}

ChatWidget::~ChatWidget()
{
	if(_model != 0)
	{
		delete _model;
		_model = 0;
	}

	if(_file != 0)
	{
		delete _file;
		_file = 0;
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

		ui->chatEdit->clear();
		ui->chatEdit->setFocus();
	}
}

void ChatWidget::addMessage(const QString &nickName, const QString &message, const QTime &time, bool saveToHistory)
{
	if(!message.isEmpty())
	{
		ChatItem *item = new ChatMessageItem(nickName, message, time, Qt::red);

		if(item != 0)
		{
			addItem(item, saveToHistory);
		}
	}
}

void ChatWidget::addItem(ChatItem *item, bool saveToHistory)
{
	int row = _model->addItem(item);
	ui->tableView->resizeRowToContents(row);

	QScrollBar *scroll = ui->tableView->verticalScrollBar();
	scroll->setValue(scroll->maximum());

	if(saveToHistory)
	{
		_chatFile << (unsigned short)item->type() << (unsigned short)item->version();
		item->write(_chatFile);
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

void ChatWidget::loadChatHistory()
{
	if(_chatFile.device()->isOpen())
	{
		while(!_chatFile.device()->atEnd())
		{
			unsigned short itemType;
			unsigned short version;

			_chatFile >> itemType >> version;

			ChatItem *item = ChatItem::createItem(static_cast<ChatItem::ItemType>(itemType));
			item->read(_chatFile, version);

			addItem(item, false);
		}
	}
}

void ChatWidget::resizeEvent(QResizeEvent *)
{
	ui->tableView->setColumnWidth(1, ui->tableView->width() - (NICKNAME_COLUMN_WIDTH + TIME_COLUMN_WIDTH + 60));
	ui->tableView->resizeRowsToContents();
}
