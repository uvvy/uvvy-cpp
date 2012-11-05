#pragma once

#include <QObject>

class QDir;
class QFileSystemWatcher;

/**
 * File watcher and automatic file tree synchronizer.
 */
class FileSync : public QObject
{
	Q_OBJECT

	QFileSystemWatcher* watcher;

	void watchPath(const QDir& path, bool isDir);

public:
	FileSync(QObject* parent = 0);

signals:
	/**
	 * A file has been added under watched tree.
	 * Signal may be emitted several times for a given path.
	 * @param path Full path to the added file.
	 */
	void fileAdded(const QString& path);
	/**
	 * A file has been removed under watched tree.
	 * Signal may be emitted several times for a given path.
	 * @param path Full path to the removed file.
	 */
	void fileRemoved(const QString& path);
	/**
	 * A file contents or metadata has been modified under watched tree.
	 * Signal may be emitted several times for a given path.
	 * @param path Full path to the modified file.
	 */
	void fileModified(const QString& path);
	/**
	 * A directory has been added under watched tree.
	 * Signal may be emitted several times for a given path.
	 * @param path Full path to the added directory.
	 */
	void directoryAdded(const QString& path);
	/**
	 * A directory has been removed under watched tree.
	 * Signal may be emitted several times for a given path.
	 * @param path Full path to the removed directory.
	 */
	void directoryRemoved(const QString& path);
	/**
	 * A directory contents or metadata has been modified under watched tree.
	 * Signal may be emitted several times for a given path.
	 * @param path Full path to the modified directory.
	 */
	void directoryModified(const QString& path);

private slots:
	void fileChanged(const QString& path);
	void directoryChanged(const QString& path);
};
