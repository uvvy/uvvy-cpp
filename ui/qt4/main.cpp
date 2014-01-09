#include <QApplication>
#include "MainWindow.h"
#include "PeerTableModel.h"
#include "logging.h"
#include "host.h"
#include "settings_provider.h"

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
    // bool verbose_debug{true};

    // if (!verbose_debug) {
        // logger::set_verbosity(logger::verbosity::info);
    // }

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    std::shared_ptr<ssu::host> host =
        ssu::host::create(settings_provider::instance());
    PeerTableModel* model = new PeerTableModel(host);

    MainWindow* mainWin = new MainWindow(model);
    mainWin->show();
    return app.exec();
}

