#include <QFileSystemWatcher>
#include <QDir>
#include <QDebug>
#include "filesync.h"
#include "env.h"

//=====================================================================================================================
// FileWatcher
//=====================================================================================================================

FileWatcher::FileWatcher(QObject* parent)
	: QObject(parent)
	, ignoreChanges(false)
	, files()
	, directories()
	, watcher(0)
{
	watcher = new QFileSystemWatcher(this);
	connect(watcher, SIGNAL(fileChanged(const QString&)),
		this, SLOT(fileChanged(const QString&)));
	connect(watcher, SIGNAL(directoryChanged(const QString&)),
		this, SLOT(directoryChanged(const QString&)));

	watchPath(shareDir, true);
}

void FileWatcher::populateDirectoryWatch(const QString& path)
{
	QDir d(path);
	d.setFilter(QDir::AllDirs|QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Hidden);
	QStringList dirs = d.entryList();
	foreach(QString dir, dirs)
	{
		QString dpath = path + "/" + dir;
		// qDebug() << "Adding watch on directory" << dpath;
		watchPath(QDir(dpath), true);
	}

	d.setFilter(QDir::Files|QDir::Hidden);
	QStringList files = d.entryList();
	foreach(QString file, files)
	{
		if (file == ".DS_Store") // @todo if file in metadata_filenames: pass
			continue;
		QString fpath = path + "/" + file;
		// qDebug() << "Adding watch on file" << fpath;
		watchPath(QDir(fpath), false);
	}
}

/*
 * Qt docs:
 * Note that QFileSystemWatcher stops monitoring files once they have been renamed or removed from disk,
 * and directories once they have been removed from disk.
 */
/*
 * To track directory or file renames, keep merkle tree of directory hashes and compare newly added directory or file
 * hash with existing one - if match exists, it's a rename.
 */

void FileWatcher::watchPath(const QDir& path, bool isDir)
{
	// qDebug() << "FILES" << files << "DIRECTORIES" << directories;

	QString pathname = path.path();
	if (isDir)
	{
		if (!path.exists())
		{
			emit directoryRemoved(pathname);
			directories.removeAll(pathname);
		}
		else
		if (!directories.contains(pathname))
		{
			emit directoryAdded(pathname);
			watcher->addPath(pathname);
			directories.append(pathname);
			populateDirectoryWatch(pathname);
		}
		else
		{
			emit directoryModified(pathname);
			populateDirectoryWatch(pathname);
		}
	}
	else
	{
		if (!path.exists(pathname))
		{
			emit fileRemoved(pathname);
			files.removeAll(pathname);
		}
		else
		if (!files.contains(pathname))
		{
			emit fileAdded(pathname);
			watcher->addPath(pathname);
			files.append(pathname);
		}
		else
		{
			emit fileModified(pathname);
		}
	}
}

void FileWatcher::fileChanged(const QString& path)
{
	if (ignoreChanges)
		return;

	ignoreChanges = true;
	// qDebug() << "FILE CHANGE NOTIFY IN" << path;
	watchPath(QDir(path), false);
	ignoreChanges = false;
}

void FileWatcher::directoryChanged(const QString& path)
{
	if (ignoreChanges)
		return;

	ignoreChanges = true;
	// qDebug() << "DIRECTORY CHANGE NOTIFY IN" << path;
	watchPath(QDir(path), true);
	ignoreChanges = false;
}

//=====================================================================================================================
// FileSync
//=====================================================================================================================

FileSync::FileSync(QObject* parent)
	: QObject(parent)
{
	watcher = new FileWatcher(this);
	connect(watcher, SIGNAL(fileAdded(const QString&)),
		this, SLOT(fileAdded(const QString&)));
	connect(watcher, SIGNAL(fileRemoved(const QString&)),
		this, SLOT(fileRemoved(const QString&)));
	connect(watcher, SIGNAL(fileModified(const QString&)),
		this, SLOT(fileModified(const QString&)));
	connect(watcher, SIGNAL(directoryAdded(const QString&)),
		this, SLOT(directoryAdded(const QString&)));
	connect(watcher, SIGNAL(directoryRemoved(const QString&)),
		this, SLOT(directoryRemoved(const QString&)));
	connect(watcher, SIGNAL(directoryModified(const QString&)),
		this, SLOT(directoryModified(const QString&)));
}

// @todo Go via Index::store(), also see Share/ShareFile, ChunkShare. ShareFile::update()

void FileSync::fileAdded(const QString& path)
{
	qDebug() << "fileAdded" << path;
}

void FileSync::fileRemoved(const QString& path)
{
	qDebug() << "fileRemoved" << path;
}

void FileSync::fileModified(const QString& path)
{
	qDebug() << "fileModified" << path;
}

void FileSync::directoryAdded(const QString& path)
{
	qDebug() << "directoryAdded" << path;
}

void FileSync::directoryRemoved(const QString& path)
{
	qDebug() << "directoryRemoved" << path;
}

void FileSync::directoryModified(const QString& path)
{
	qDebug() << "directoryModified" << path;
}
