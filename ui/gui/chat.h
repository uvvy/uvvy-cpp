#pragma once

#include <QList>
#include <QLineEdit>
#include <QPointer>
#include <QDialog>
#include <QProgressDialog>
#include "stream.h"
#include "scan.h"
#include "peerid.h"

class QLabel;
class QGridLayout;
class QLineEdit;
class QPushButton;
class LogArea;
class ChatServer;
class FileInfo;
class ChatHistory;

struct ChatProtocol
{
	// Message type codes
	enum MsgCode {
		Invalid = 0,
		Text,			// Plain text message (UTF-8)
		Files,			// File shares (encoded FileInfo)
	};
};

class ChatDialog : public QDialog, public ChatProtocol
{
	friend class ChatServer;
	Q_OBJECT

	ChatDialog(const SST::PeerId& id, const QString &name, SST::Stream *strm = NULL);
	~ChatDialog();

	const SST::PeerId otherid;	// Who we're chatting with
	const QString othername;

	SST::Stream *stream;		// Connection we chat over

	ChatHistory* history;       // History storage for this chat

	QList<QLabel*> labels;		// List of log entry sender labels
	QList<QWidget*> entries;	// List of log entry display widgets

	QWidget *logwidget;		// Virtual widget displaying the log
	QGridLayout *loglayout;

	LogArea *logview;		    // Scrolling view onto the logwidget
	QLineEdit *textentry;		// Text entry line
	QPushButton *button;        // Send button

	// Global hash table of open chat dialogs by ID
	//static QHash<QByteArray, ChatDialog*> chathash;

public:

	static ChatDialog *open(const SST::PeerId &id, const QString &name);

	void sendFiles(const QList<FileInfo> &files);


private:
	inline bool active() { return !textentry->isReadOnly(); }

	void closeEvent(QCloseEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);

	void addText(const QString &source, const QString &text);
	void addFiles(const QString &source, const QList<FileInfo> &files);
	void addEntry(QString source, QWidget *widget);

private slots:
	void connected();
	void readyReadMessage();
	void streamError(const QString &err);
	void sendTextLine();
	/**
	 * Load available chat history. Currently tries to load everything, for simplicity.
	 */
	void loadHistory();
};


// Helper class for receiving incoming chat requests
class ChatServer : public SST::StreamServer
{
	Q_OBJECT

public:
	ChatServer(QObject *parent = NULL);

private slots:
	void incoming();
};


// Helper class to scan and send a file or directory tree
class ChatScanner : public QProgressDialog
{
	Q_OBJECT

private:
	ChatDialog *chat;
	QList<Scan*> scans;

public:
	ChatScanner(ChatDialog *parent, const QStringList &files);

private slots:
	void scanProgress();
	void scanCanceled();
};

