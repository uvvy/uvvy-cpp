#pragma once

#include <QtGui>
#include "ui_PeerPickerWindow.h"

class PeerPicker : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    PeerPicker(QWidget *parent = 0);
 
public slots:
    void quit();
    void chat();
    void call();
    void addToFavorites();
};
