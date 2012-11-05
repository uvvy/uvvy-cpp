#ifndef SAVE_H
#define SAVE_H

#include <QDialog>
#include <QFileDialog>

#include "file.h"

class QWidget;
class QLabel;
class QProgressBar;
class QPushButton;
class QGridLayout;
class LogArea;
class SaveItem;
class Update;
class Action;	// lawsuit


// Firefox-esque dialog showing progress of current and recent downloads
class SaveDialog : public QDialog
{
	friend class SaveItem;
	Q_OBJECT

private:
	// Download list format:
	// status label, progress bar, cancel button
	static const int columns = 3;

	QList<SaveItem*> items;		// List of active items

	QWidget *listwidget;		// Virtual widget displaying the items
	QGridLayout *listlayout;
	LogArea *listview;		// Scrolling view onto the logwidget


public:
	static void init();
	static void save(const FileInfo &info, const QString &localName);
	static void present();

	static int numActive();

private:
	SaveDialog();
	void writestate();
	void relayout();

private slots:
	void cleanup();
};

// Private helper class for SaveDialog, representing one download item
class SaveItem : public QObject
{
	friend class SaveDialog;
	Q_OBJECT

private:
	static const int progressMax = 1000000;

	const FileInfo info;
	const QString localpath;
	QString finalmsg;
	Action *act;
	QLabel *status;
	QProgressBar *progress;
	//QPushButton *pause;
	QPushButton *cancel;


	SaveItem(SaveDialog *dlg, const FileInfo &info,
		const QString &localpath, const QString &finalmsg = QString());
	~SaveItem();

	bool isDone();

private slots:
	void pausePressed();
	void cancelPressed();
	void updateStatus();
	void updateProgress(float ratio);
};

// A modeless file dialog for picking a download location
class SaveAsDialog : public QFileDialog
{
	Q_OBJECT

private:
	const FileInfo info;			// Keys to file to be saved
	bool done;

public:

	// Interactively start a Save As... operation for a specified file,
	// requesting a local filename from the user and starting a download.
	static void saveAs(const FileInfo &info);

private:
	SaveAsDialog(const FileInfo &info);

private slots:
	void gotFinished(int result);
};

#endif	// SAVE_H
