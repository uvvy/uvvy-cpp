#ifndef MACSUPPORT_H
#define MACSUPPORT_H

#include <QObject>

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

#endif
