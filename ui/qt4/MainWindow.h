#pragma once

#include <QMainWindow>
#include <QModelIndex>
#include <QSystemTrayIcon>

class QTableView;
class PeerTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QTableView *peerlist;

public:
    MainWindow(PeerTableModel* model);
    ~MainWindow();

protected:
    // Re-implement to watch for window activate/deactivate events.
    virtual bool event(QEvent *event);
    virtual void closeEvent(QCloseEvent *event);

private:
    int selectedFriend();

private slots:
    void openAbout();
    void exitApp();
    void updateMenus();
    void friendsClicked(const QModelIndex &index);
    void trayActivate(QSystemTrayIcon::ActivationReason);
};

