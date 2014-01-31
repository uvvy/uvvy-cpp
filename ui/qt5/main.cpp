// #include "arsenal/logging.h"
#include "ssu/host.h"
#include "arsenal/settings_provider.h"
#include "MainWindow.h"
#include "PeerTableModel.h"
#include "XcpApplication.h"

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
    // bool verbose_debug{true};

    // if (!verbose_debug) {
        // logger::set_verbosity(logger::verbosity::info);
    // }

    XcpApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    std::shared_ptr<ssu::host> host =
        ssu::host::create(settings_provider::instance());
    PeerTableModel* model = new PeerTableModel(host);

    MainWindow* mainWin = new MainWindow(model);
    mainWin->show();
    return app.exec();
}

