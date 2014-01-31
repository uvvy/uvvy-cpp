#pragma once

#include <QGuiApplication>

class XcpApplication : public QGuiApplication
{
public:
    XcpApplication(int &argc, char *argv[]) : QGuiApplication(argc, argv) {}
    bool notify(QObject *receiver_, QEvent *event_) override;
};
