#include <QCoreApplication>
#include "PeerPicker.h"

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    PeerPicker *dlg = new PeerPicker;
    dlg->show();
    return app.exec();
}
