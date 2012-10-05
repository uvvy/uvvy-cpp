// Local file index, providing:
// - lookup by chunk hash of both file data chunks and metadata chunks
// - lookup of the FileInfo representing a local file, by local pathname
//
// This module is thread-safe: can be called by different threads at any time.
// It is a purely passive data repository, however:
// higher layers are responsible for tracking file system changes.
//
#ifndef INDEX_H
#define INDEX_H

#include "store.h"

class FileInfo;


class Index : public Store
{
	// Private constructor, to enforce only one instance
	Index();

	virtual QByteArray readStore(const QByteArray &ohash);

public:

	// Create if necessary and return the Index object instance.
	static Index *store();
	static inline void init() { (void)store(); }

	// Register and retrieve metadata about scanned files
	static bool addFileInfo(const QString &path, const FileInfo &info);
	static FileInfo getFileInfo(const QString &path);

	// Register a data chunk existing in a local file.
	static bool addFileChunk(const QByteArray &ohash, const QString &path,
				qint64 ofs, int len);

	// Insert a metadata chunk's raw cyphertext into the chunk store.
	static bool addMetaChunk(const QByteArray &ohash,
				const QByteArray &cyph);

	// Update the file index to reflect a file rename.
	static void fileMoved(const QString &oldpath, const QString &newpath);

	static QString errorString();

private:
	static QByteArray readFileChunk(const QByteArray &ohash);
	static QByteArray readMetaChunk(const QByteArray &ohash);

private slots:
	void flushTimeout();
};


#endif	// INDEX_H
