#pragma once

#include <QObject>

class QFileSystemWatcher;

/**
 * File watcher and automatic file tree synchronizer.
 */
class FileSync : public QObject
{
	Q_OBJECT

	QFileSystemWatcher* watcher;

public:
	FileSync(QObject* parent = 0);

private slots:
	void fileChanged(const QString& path);
	void directoryChanged(const QString& path);
};
