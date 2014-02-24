#include "ContactModel.h"
#include "MainWindow.h"
#include "XcpApplication.h"
#include "HostState.h"
#include <QtCore/QUrl>
#include <QDebug>

//
// Main application entrypoint
//
int main(int argc, char* argv[])
{
    // bool verbose_debug{true};
    // if (!verbose_debug) {
        // logger::set_verbosity(logger::verbosity::info);
    // }
    HostState state;
    ContactModel* model = new ContactModel(state.host(), state.settings());

    XcpApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    MainWindow win(model);
    win.show();

    return app.exec();
}
