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

	watchPath(shareDir, true);
}

void FileSync::watchPath(const QDir& path, bool isDir)
{

	watcher->addPath(path.path());
	QDir d(path);
	d.setFilter(QDir::AllDirs|QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Hidden);
	QStringList dirs = d.entryList();
	foreach(QString dir, dirs)
	{
		QString dpath = path.path() + "/" + dir;
		qDebug() << "Adding watch on directory" << dpath;
		watcher->addPath(dpath);
	}

	d.setFilter(QDir::Files|QDir::Hidden);
	QStringList files = d.entryList();
	foreach(QString file, files)
	{
		if (file == ".DS_Store")
			continue;
		QString fpath = path.path() + "/" + file;
		qDebug() << "Adding watch on file" << fpath;
		watcher->addPath(fpath);
	}
}

void FileSync::fileChanged(const QString& path)
{
	qDebug() << "FILE CHANGE NOTIFY IN" << path;
	QStringList origFiles = watcher->files();
	watchPath(QDir(path), false);
}

void FileSync::directoryChanged(const QString& path)
{
	qDebug() << "DIRECTORY CHANGE NOTIFY IN" << path;
	QStringList origDirs = watcher->directories();
	watchPath(QDir(path), true);
}
