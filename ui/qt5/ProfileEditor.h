#pragma once

#include <QtGui>
#include "ui_ProfileEditor.h"

/**
 * Simple client_profile editor.
 * Open settings_provider with default settings (or a specified settings file),
 * display current data and allow editing profile fields using Qt4 gui.
 * Save profile back.
 */
class ProfileEditor : public QWidget, private Ui::ProfileEditor
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    ProfileEditor(QWidget *parent = 0);

signals:
    void profileChanged();

public slots:
    void load();
    void save();
    void generateNewEid();
    void setStatus(QString const& text);
    void clearStatus();
};
