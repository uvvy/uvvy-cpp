#pragma once

#include <QObject>

//=================================================================================================
// MacSupport
// Handles Dock interaction and requesting App attention.
//=================================================================================================

class MacSupport : public QObject
{
    Q_OBJECT
public:
    MacSupport();
    virtual ~MacSupport();
    void emitDockClick();

public slots:
    void setDockBadge(const QString & badgeText);
    void requestAttention();

signals:
    void dockClicked();
};

