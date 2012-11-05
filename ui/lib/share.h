#pragma once

#include <QSet>
#include <QList>
#include <QHash>
#include <QFlags>
#include <QString>
#include <QDateTime>

#include "file.h"

class Share;
class ShareFile;

/**
Sharing of files and directory trees on the local file system.
This is the basis for filesync/backup.
*/
class Share : public QObject
{
	friend class ShareFile;
	Q_OBJECT

	const QString &root;
	QSet<ShareFile*> files;

public:
	Share(QObject *parent, const QString &root);
	~Share();

	// Overridable method to determine whether to exclude a given subtree.
	// The default implementation always returns false.
	virtual bool exclude(const QString &path);

private:
	void insert(ShareFile *sf, bool recursive = true);
	void remove(ShareFile *sf, bool recursive = true);
};


/**
 * The Share class represents a shareable object on a local file system.
 */
class ShareFile : public QObject
{
	friend class Share;
	Q_OBJECT

	const QString path;
	QSet<Share*> shares;	// Shares interested in this file
	QHash<QString, ShareFile*> subs;
	FileInfo info;
	QDateTime modtime;


public:

	// Called when a possible change has been detected to the file,
	// meaning it probably needs to be re-scanned.
	//void markRescan() { }	// XXX

	// Returns the FileInfo metadata describing this file.
	inline FileInfo fileInfo() { return info; }
	inline QString filePath() { return path; }
	QString fileName();	// Last component only
	inline QDateTime lastModified() { return modtime; }

	// Update the tree summary information for this file.
	void update(const FileInfo &newInfo, const QDateTime newModTime);

	// Find the parent directory containing this ShareFile, if any.
	// Returns NULL if none or not yet created.
	ShareFile *parent();

	// Find or create if necessary the ShareFile for a given canonical path
	static ShareFile *file(const QString &canonPath, bool create);

signals:
	void updated();
	void newChild(const QString &path);

private:
	ShareFile(const QString &path);
	~ShareFile();

	void checkparent();
};
