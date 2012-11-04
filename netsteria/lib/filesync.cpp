#include <QFileSystemWatcher>
#include <QDir>
#include <QDebug>
#include "filesync.h"
#include "env.h"

FileSync::FileSync(QObject* parent)
	: QObject(parent)
{
	watcher = new QFileSystemWatcher(this);
	connect(watcher, SIGNAL(fileChanged(const QString&)),
		this, SLOT(fileChanged(const QString&)));
	connect(watcher, SIGNAL(directoryChanged(const QString&)),
		this, SLOT(directoryChanged(const QString&)));
	watcher->addPath(shareDir.path());
}

void FileSync::fileChanged(const QString& path)
{
	qDebug() << "FILE CHANGE NOTIFY IN" << path;
}

void FileSync::directoryChanged(const QString& path)
{
	qDebug() << "DIRECTORY CHANGE NOTIFY IN" << path;
}
