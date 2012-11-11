#include "udpsend.h"
#include <QCoreApplication>

//
// Main application entrypoint
//
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
    UdpTestSender sock;
    sock.ping(QHostAddress("212.7.7.70"), 9660);
	return app.exec();
}
