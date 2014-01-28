#include <iostream>
#include "XcpApplication.h"

bool XcpApplication::notify(QObject *receiver_, QEvent *event_)
{
    try {
        return QApplication::notify(receiver_, event_);
    }
    catch (std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        qFatal("Unhandled application exception.");
    }
    return false;
}
