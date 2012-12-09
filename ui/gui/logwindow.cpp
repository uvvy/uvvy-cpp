#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTreeView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QtDebug>

#include "logwindow.h"
#include "logarea.h"
#include "env.h"

LogWindow::LogWindow()
    : QDialog(NULL)
{
    setWindowTitle(tr("Log"));

    settings->beginGroup("LogWindow");
    move(settings->value("pos", QPoint(100, 100)).toPoint());
    resize(settings->value("size", QSize(400, 500)).toSize());
    settings->endGroup();

    logwidget = new QWidget(this);
    loglayout = new QGridLayout();
    logwidget->setLayout(loglayout);

    logview = new LogArea(this);
    logview->setWidget(logwidget);
    logview->setWidgetResizable(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(2,2,2,2);
    layout->setSpacing(1);
    layout->addWidget(logview);
    setLayout(layout);
}

LogWindow::~LogWindow()
{
    qDebug() << "~LogWindow";
}

void LogWindow::closeEvent(QCloseEvent *event)
{
    settings->beginGroup("LogWindow");
    settings->setValue("pos", pos());
    settings->setValue("size", size());
    settings->endGroup();

    // Close our window as usual
    QDialog::closeEvent(event);
}

void LogWindow::writeLog(const QString &text)
{
    QLabel *disp = new QLabel(text, logwidget);
    disp->setWordWrap(true);

    // Add the label and entry widget to the log.
    int itemno = entries.size();
    entries.append(disp);

    // Add them to the log's layout.
    loglayout->addWidget(disp, itemno, 1);
    loglayout->setRowStretch(itemno+1, 1);
}

LogWindow& LogWindow::get()
{
    static LogWindow* window = 0;
    if (!window)
        window = new LogWindow;
    return *window;
}
