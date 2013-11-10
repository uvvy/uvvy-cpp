#include <QApplication> 
#include "profile_editor.h"
 
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow *dlg = new MainWindow;
    dlg->show();
    return app.exec();
}
