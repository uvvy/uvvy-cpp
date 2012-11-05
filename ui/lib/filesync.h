#pragma once

#include <QObject>
#include <QStringList>

class QDir;
class QFileSystemWatcher;

/**
 * File watcher and automatic file tree synchronizer.
 */
class FileWatcher : public QObject
{
	Q_OBJECT

	bool ignoreChanges;
	QStringList files, directories;
	QFileSystemWatcher* watcher;

	void watchPath(const QDir& path, bool isDir);
	void populateDirectoryWatch(const QString& path);

public:
	FileWatcher(QObject* parent = 0);

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

class FileSync : public QObject
{
	Q_OBJECT

	FileWatcher* watcher;

public:
	FileSync(QObject* parent = 0);

public slots:
	// @todo Should track more sophisticated changes like file renames.

	/**
	 * A file has been added under watched tree.
	 * Initiate metadata (and data - @fixme only metadata should suffice!) sync operation to peer nodes.
	 * @param path Full path to the added file.
	 */
	void fileAdded(const QString& path);
	/**
	 * A file has been removed under watched tree.
	 * Initiate remove operation on peer nodes.
	 * @param path Full path to the removed file.
	 */
	void fileRemoved(const QString& path);
	/**
	 * A file contents or metadata has been modified under watched tree.
	 * Initiate data sync operation to peer nodes.
	 * @param path Full path to the modified file.
	 */
	void fileModified(const QString& path);
	/**
	 * A directory has been added under watched tree.
	 * Initiate directory merkle trees resync with peer nodes.
	 * @param path Full path to the added directory.
	 */
	void directoryAdded(const QString& path);
	/**
	 * A directory has been removed under watched tree.
	 * Initiate remove operation on peer nodes.
	 * @param path Full path to the removed directory.
	 */
	void directoryRemoved(const QString& path);
	/**
	 * A directory contents or metadata has been modified under watched tree.
	 * Initiate data sync operation to peer nodes.
	 * @param path Full path to the modified directory.
	 */
	void directoryModified(const QString& path);
};
