#ifndef MAIN_H
#define MAIN_H

#include <QString>
#include <QByteArray>
#include <QMainWindow>
#include <QList>
#include <QSystemTrayIcon>

#include "regcli.h"

class QSettings;
class QDir;
class QTableView;
class QModelIndex;
class QAction;

class SearchDialog;
class ChatDialog;
class PeerTable;

// Columns in PeerTable
#define NCOLS		6
// default ones
#define COL_NAME    0
#define COL_EID     1
// custom
#define COL_ONLINE  2
#define COL_FILES   3
#define COL_TALK	4
#define COL_LISTEN	5

class MainWindow : public QMainWindow
{
	Q_OBJECT

	SearchDialog *searcher;
	QTableView *friendslist;

	// Menu/toolbar actions requiring dynamic enable/disable control
	QAction *maMessage, *maTalk, *maRename, *maDelete;
	QAction *taMessage, *taTalk, *taRename, *taDelete;


public:
	MainWindow();
	~MainWindow();

	// Add identity 'id' as a friend with suggested name 'name'.
	// If 'edit' is true, start editing the name after adding it.
	void addPeer(const QByteArray &id, QString name, bool edit = false);

protected:
	// Re-implement to watch for window activate/deactivate events.
	virtual bool event(QEvent *event);

	virtual void closeEvent(QCloseEvent *event);

private:
	int selectedFriend();

private slots:
	void friendsClicked(const QModelIndex &index);
	void trayActivate(QSystemTrayIcon::ActivationReason);
	void openFriends();
	void openSearch();
	void openChat();
	void openDownload();
	void openSettings();
	void openProfile();
	void gotoFiles();
	void openHelp();
	void openWeb();
	void openAbout();
	void startTalk();
	void renameFriend();
	void deleteFriend();
	void updateMenus();
	void exitApp();
};


extern MainWindow *mainwin;
extern PeerTable *friends;

extern QList<SST::RegClient*> regclients;
extern SST::RegInfo myreginfo;

extern QSettings *settings;
extern QDir appdir;



#endif	// MAIN_H
