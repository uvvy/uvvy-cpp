#ifndef ARCH_H
#define ARCH_H

#include <QString>
#include <QVector>
#include <QMultiHash>
#include <QFile>

#include "store.h"
#include "chunk.h"

class QDir;


// Type for chunk index numbers within an archive
typedef qint32 ArchiveIndex;

class Archive : public Store
{
	static const int hdrsize = 16;	// Size of chunk header
	static const int chksize = 8;	// Size of check field

	QFile file;
	qint64 readpos;
	ArchiveIndex readidx;
	bool verify;


	// Index of chunk header positions in the archive file.
	QVector<quint64> chunks;

	// Hash table to lookup chunks by outer hash or check.
	// Collisions possible since OuterCheck is only 64 bits,
	// so we use QMultiHash and allow multiple values per key.
	QMultiHash<OuterCheck, ArchiveIndex> chash;


	Archive(const QString &filename);

	// Read a chunk header at a given archive file position.
	bool readHeader(quint64 pos, quint64 &check, qint32 &size);

	// Read a chunk at a given archive file position.
	QByteArray readAt(quint64 pos, quint64 expcheck = 0, int expsize = -1);

	// Read a chunk at a given archive index.
	// If size is >= 0, only read the chunk if actual size matches.
	QByteArray readChunk(ArchiveIndex idx, quint64 expcheck = 0,
				int expsize = -1);

	// Scan for new chunks written to the underlying file.
	bool scan();


	// Store method implementations
	virtual QByteArray readStore(const QByteArray &ohash);


public:
	// Read and verify the archived chunk, if present,
	// matching a specified full 512-bit outer hash.
	// May be slightly more efficient if size is provided.
	QByteArray readChunk(const QByteArray &ohash, int size = -1);

	// Write a chunk to the archive, and return its SHA-512 outer hash.
	// Returns an empty hash if the write fails (e.g., disk full).
	QByteArray writeChunk(const QByteArray &ch);

	// Test the integrity of the entire archive.
	// XXX may take a long time - supply progress reports.
	bool test();

public:
	static Archive *primary;

	static void init(const QDir &appdir);
};

#endif	// ARCH_H
