#include <QtGui>

// Save pixmap to a byte array:
// QPixmap pixmap(<image path>);
// QByteArray bytes;
// QBuffer buffer(&bytes);
// buffer.open(QIODevice::WriteOnly);
// pixmap.save(&buffer, "JPEG", 0); 

float dpiScaleFactor(QWidget* widget);

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QDesktopWidget *desktop = QApplication::desktop();
    float scale = dpiScaleFactor(desktop);
    qDebug() << "Scale factor" << scale;
    QPixmap screenshot = QPixmap::grabWindow(desktop->winId(),
        0, 0, desktop->width()*scale, desktop->height()*scale);
    QFile file("screenshot.png");
    file.open(QIODevice::WriteOnly);
    screenshot.save(&file, "JPEG", 100);//(-1..0-100) 0 smallest lowest quality
    return app.exec();
}
