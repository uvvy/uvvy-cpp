#pragma once

#include <QList>
#include <QLineEdit>
#include <QPointer>
#include <QDialog>
#include <QProgressDialog>

class QLabel;
class QGridLayout;
class QLineEdit;
class QPushButton;
class LogArea;
class ChatServer;
class FileInfo;
class ChatHistory;

class LogWindow : public QDialog
{
	Q_OBJECT

	LogWindow();
	~LogWindow();

	QWidget *logwidget;		// Virtual widget displaying the log
	QGridLayout *loglayout;
	LogArea *logview;		    // Scrolling view onto the logwidget
	QList<QWidget*> entries;	// List of log entry display widgets

public:
	static LogWindow& get();

	void writeLog(const QString &text);

private:
	void closeEvent(QCloseEvent *event);
};

inline LogWindow& operator <<(LogWindow& lw, const QString& s)
{
	lw.writeLog(s);
	return lw;
}
