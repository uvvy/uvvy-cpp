#pragma once

#include <QtGui>
#include "ui_ProfileEditor.h"

class ProfileEditor : public QMainWindow, private Ui::ProfileEditor
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    ProfileEditor(QWidget *parent = 0);

public slots:
    void load();
    void save();
    void generateNewEid();
    void setStatus(QString const& text);
    void clearStatus();
};
