#include "Root.h"

#include <QtWidgets/QApplication>
#include "Chat/ChatWidget.h"

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

	/*
    QApplication app(argc, argv);

	ChatWidget::setChatDirectory("/tmp");
	ChatWidget w("1294883090");
	w.show();

	*/

    gui::Root s;
    return app.exec();
}
