#pragma once

#include <QtGui>
#include "ui_main_window.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    MainWindow(QWidget *parent = 0);
 
public slots:
    void load();
    void save();
    void quit();
    void generateNewEid();
    void setStatus(QString const& text);
    void clearStatus();
};
