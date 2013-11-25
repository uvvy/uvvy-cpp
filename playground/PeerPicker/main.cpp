#include <QCoreApplication>
#include "PeerPicker.h"
#include "logging.h"

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
    bool verbose_debug{false};

    if (!verbose_debug) {
        logger::set_verbosity(logger::verbosity::info);
    }

    QApplication app(argc, argv);
    PeerPicker *dlg = new PeerPicker;
    dlg->show();
    return app.exec();
}
