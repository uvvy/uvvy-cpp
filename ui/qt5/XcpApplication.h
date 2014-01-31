#pragma once

#include <QApplication>

class XcpApplication : public QApplication
{
public:
    XcpApplication(int &argc, char *argv[]) : QApplication(argc, argv) {}
    bool notify(QObject *receiver_, QEvent *event_) override;
};
