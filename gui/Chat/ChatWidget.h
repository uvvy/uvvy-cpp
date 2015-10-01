#pragma once

/*
 * Widget ChatWidget implements chat abilities.
 * Use setID() to set user id. 
 * Signal sendMessage(id, text, time) appears when current user adds some message.
 * ChatWidget doesn't add anything to itself. Use addMessage and other methods
 * to add messages (or information about calls/missed calls) to chat widget
 * */

class Ui_Form;

class ChatItemModel;
class ChatItemDelegate;

class ChatWidget: public QWidget
{
	Q_OBJECT

private:
	enum EntryType
	{
		CHAT_MESSAGE,
		CHAT_OWN_MESSAGE,
		CHAT_CALL,
		CHAT_MISSED_CALL
	};

public:
	ChatWidget(const QString &id, QWidget *parentWidget = 0);
	~ChatWidget();

	static void setChatDirectory(const QString &chatDirectory);

	static const QString &chatDirectory();

	const QString &id()
	{
		return _id;
	}

	void addMessage(const QString &nickName, const QString &message, const QTime &time, bool saveToHistory = true);

	void resizeEvent(QResizeEvent *event);

private:
	Ui_Form *ui;

public slots:
	// Internal slot for internal widgets. Don't use this
	void widgetSendMessage();

	void saveEntry(EntryType type, const QString &nickName, const QString &text, const QTime &time);

signals:
	void sendMessage(const QString &id, const QString &text, const QTime &time);

private:
	QString _id;
	QFile _chatFile;

	static QString _chatDirectory;

private:
	void loadChatHistory();
	ChatItemModel *_model = 0;
	ChatItemDelegate *_delegate = 0;
};
