
#include <QSet>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QTemporaryFile>
#include <QDateTime>
#include <QtDebug>

#include "index.h"
#include "opaque.h"
#include "file.h"
#include "sha2.h"
#include "xdr.h"
#include "env.h"

using namespace SST;


#define VXA_INDEXSIG	((qint64)0x565841696e646578LL)	// 'VXAindex'
#define VXA_INDEXVER	((qint32)2)

enum IndexTag {
	FileChunkTag	= 1,
	MetaChunkTag	= 2,
	FileInfoTag	= 3,
};

#define FLUSHTIME	1000		// flush every 1 sec


struct FileChunk {
	QString path;
	qint64 ofs;
	qint32 len;
};

static QFile indexfile;
static QString chunksprefix;

static QHash<QString, FileInfo> infohash;
static QHash<QByteArray, QString> metachunks;
static QHash<QByteArray, QSet<FileChunk> > filechunks;

static QString indexerr;

static QMutex mutex;


////////// Support functions //////////

inline bool operator==(const FileChunk &fc1, const FileChunk &fc2)
{
	return fc1.path == fc2.path && fc1.ofs == fc2.ofs && fc1.len == fc2.len;
}

inline uint qHash(const FileChunk &fc)
{
	return qHash(fc.path) + qHash(fc.ofs) + qHash(fc.len);
}


////////// Index //////////

Index *Index::store()
{
	static Index *st;
	if (st == NULL) {
		QMutexLocker lock(&mutex);
		if (st == NULL)
			st = new Index();
	}
	return st;
}

Index::Index()
{
	qDebug() << "Index: initializing";

	QString apppath = appdir.path();
	if (apppath.isEmpty())
		qFatal("No application path - cannot create local file index");

	// Open or create the index file
	QString indexpath = apppath + "/index";
	indexfile.setFileName(indexpath);
	if (!indexfile.open(QIODevice::ReadWrite | QIODevice::Append))
		qFatal("Can't open file index '%s': %s",
			indexpath.toLocal8Bit().data(),
			indexfile.errorString().toLocal8Bit().data());

	// Make sure the chunks directory exists
	QString chunkspath = apppath + "/chunks";
	QDir().mkdir(chunkspath);
	if (!QDir(chunkspath).exists())
		qFatal("Can't create chunk index directory '%s'",
			chunkspath.toLocal8Bit().data());
	chunksprefix = chunkspath + '/';

	if (indexfile.size() == 0) {

		// Creating a new index, or re-create an unreadable index.
		qDebug() << "Index: creating new index";

		// First clear the file.
		create:
		indexfile.reset();
		indexfile.resize(0);
		metachunks.clear();
		filechunks.clear();

		// Write the new index header.
		XdrStream ws(&indexfile);
		ws << VXA_INDEXSIG << VXA_INDEXVER;
		if (ws.status() != ws.Ok)
			qFatal("Can't initialize file index '%s': %s",
				indexpath.toLocal8Bit().data(),
				ws.errorString().toLocal8Bit().data());

		// Clear out the chunks directory for good measure
		QStringList ents = QDir(chunkspath).entryList(QDir::Files);
		foreach (const QString &ent, ents) {
			QString entpath = chunksprefix + ent;
			qDebug() << "Removing old chunk file" << entpath;
			QFile::remove(entpath);
		}

	} else {

		// Read the existing index header
		indexfile.seek(0);
		XdrStream rs(&indexfile);
		qint64 sig;
		qint32 ver;
		rs >> sig >> ver;
		if (rs.status() != rs.Ok) {
			qDebug() << "Can't read existing file index:"
				<< rs.errorString();
			goto create;
		}
		if (sig != VXA_INDEXSIG) {
			qDebug() << "Existing file index has bad signature - "
				"building new index";
			goto create;
		}
		if (ver != VXA_INDEXVER) {
			qDebug() << "Existing file index has wrong version - "
				"building new index";
			goto create;
		}
	}
	Q_ASSERT(indexfile.pos() == 12);

	// Read the existing contents of the index file
	XdrStream rs(&indexfile);
	int nrecs = 0;
	while (!indexfile.atEnd()) {

		// Decode the index record tag
		qint32 tag;
		rs >> tag;
		if (rs.status() == rs.ReadPastEnd) {
			qDebug() << "Index: atEnd() didn't detect the end!";
			break;
		}
		if (rs.status() != rs.Ok) {
			corrupt:
			qDebug() << "File index corruption detected"
				<< "at position" << indexfile.pos()
				<< "tag" << tag
				<< "- building new index";
			goto create;
		}

		// Decode and process the record
		switch ((IndexTag)tag) {

		case FileChunkTag: {
			QByteArray ohash;
			FileChunk fc;
			rs >> ohash >> fc.path >> fc.ofs >> fc.len;
			if (rs.status() != rs.Ok || ohash.isEmpty()
					|| fc.path.isEmpty() || fc.len <= 0)
				goto corrupt;
			filechunks[ohash].insert(fc);
			break; }

		case MetaChunkTag: {
			QByteArray ohash;
			QString name;
			qint32 len;
			rs >> ohash >> name >> len;
			if (rs.status() != rs.Ok || ohash.isEmpty()
					|| name.isEmpty() || len <= 0)
				goto corrupt;

			// Make sure the metadata chunk file still exists
			QFileInfo fi(chunksprefix + name);
			if (fi.exists() && fi.isFile() && fi.size() == len)
				metachunks.insert(ohash, name);
			else
				qDebug() << "Missing or bad metadata file";
			break; }

		case FileInfoTag: {
			QString path;
			FileInfo info;
			rs >> path >> info;
			if (rs.status() != rs.Ok || path.isEmpty()
					|| info.isNull())
				goto corrupt;
			qDebug() << "Index: FileInfo for path" << path
				<< "modtime" << info.modTime().toString();
			infohash.insert(path, info);
			break; }

		default:
			goto corrupt;
		}
	}
	qDebug() << "Index: read" << nrecs << "records";

	indexfile.flush();
}

bool Index::addFileInfo(const QString &path, const FileInfo &info)
{
	init();
	Q_ASSERT(indexfile.isOpen());

	QMutexLocker lock(&mutex);

	if (infohash.value(path) == info)
		return true;		// Unchanged

	qDebug() << "Index::addFileInfo path" << path
		<< "modtime" << info.modTime().toString();

	// Add a record to the persistent index
	XdrStream ws(&indexfile);
	ws << (qint32)FileInfoTag << path << info;
	// XXX error handling

	// Make sure the data gets flushed to disk
	indexfile.flush();

	infohash.insert(path, info);
	return true;
}

FileInfo Index::getFileInfo(const QString &path)
{
	init();

	QMutexLocker lock(&mutex);
	return infohash.value(path);
}

bool Index::addFileChunk(const QByteArray &ohash, const QString &path,
				qint64 ofs, int len)
{
	init();
	Q_ASSERT(indexfile.isOpen());

	// Build a FileChunk struct representing this chunk
	FileChunk fc;
	fc.path = path;
	fc.ofs = ofs;
	fc.len = len;

	QMutexLocker lock(&mutex);

	// If we already know about it, we're done.
	if (filechunks[ohash].contains(fc))
		return true;

	// Add a chunk record to the persistent index
	XdrStream ws(&indexfile);
	ws << (qint32)FileChunkTag << ohash << fc.path << fc.ofs << fc.len;
	// XXX error handling

	// Make sure the data gets flushed to disk
	indexfile.flush();

	filechunks[ohash].insert(fc);
	return true;
}

QByteArray Index::readFileChunk(const QByteArray &ohash)
{
	// Atomically copy the FileChunks set for this ohash,
	// but don't hold the mutex longer than necessary.
	mutex.lock();
	QSet<FileChunk> chunks = filechunks.value(ohash);
	mutex.unlock();

	// Search for a chunk that's still valid
	foreach (const FileChunk &fc, chunks) {

		QFile file(fc.path);
		if (!file.open(QFile::ReadOnly)) {
			fail:
			qDebug("File %s changed out from underneath us!",
				fc.path.toLocal8Bit().data());
			mutex.lock();
			filechunks[ohash].remove(fc);
			mutex.unlock();
			continue;
		}

		if (!file.seek(fc.ofs))
			goto fail;

		QByteArray filedata = file.read(fc.len);
		if (filedata.size() != fc.len)
			goto fail;

		QByteArray cyph;
		OpaqueKey key = OpaqueCoder::encDataChunk(filedata, 0, cyph);

		if (key.ohash != ohash)
			goto fail;

		return cyph;
	}
	return QByteArray();
}

bool Index::addMetaChunk(const QByteArray &ohash,
				const QByteArray &chunkdata)
{
	Q_ASSERT(!ohash.isEmpty());
	Q_ASSERT(!chunkdata.isEmpty());

	init();
	Q_ASSERT(indexfile.isOpen());

	// If we already have this chunk in the metadata store,
	// just keep the existing copy - but verify that it's valid.
	if (!readMetaChunk(ohash).isEmpty())
		return true;
	Q_ASSERT(!metachunks.contains(ohash));

	// Create a temporary file to hold the metadata chunk
	QTemporaryFile tmp(chunksprefix + "chunk");
	if (!tmp.open()) {
		indexerr = tr("Can't create chunk metadata file in '%0': %1")
				.arg(chunksprefix).arg(tmp.errorString());
		return false;
	}

	// Write the chunk data
	if (tmp.write(chunkdata) != chunkdata.size()) {
		indexerr = tr("Can't write chunk metadata file '%0': %1")
				.arg(tmp.fileName()).arg(tmp.errorString());
		return false;	// will delete temporary file
	}

	// Determine the simple relative name of the temporary file
	QString name = tmp.fileName();
	int lastslash = name.lastIndexOf('/');
	if (lastslash >= 0)
		name = name.mid(lastslash+1);
	Q_ASSERT(name.startsWith("chunk"));

	// We're done writing the chunk
	tmp.close();

	QMutexLocker lock(&mutex);

	if (metachunks.contains(ohash)) {
		// Oops, someone else got here first while the lock was free.
		// Just delete our temporary file and return.
		return true;
	}

	// Record the chunk name in the persistent index
	Q_ASSERT(indexfile.isOpen());
	XdrStream ws(&indexfile);
	ws << (qint32)MetaChunkTag << ohash << name << (qint32)chunkdata.size();
	// XXX error handling

	// Make sure the data gets flushed to disk
	indexfile.flush();

	// Now add the chunk to our index and leave the temporary file around.
	metachunks.insert(ohash, name);
	tmp.setAutoRemove(false);
	return true;
}

QByteArray Index::readMetaChunk(const QByteArray &ohash)
{
	// Find the chunk file containing this metadata chunk
	mutex.lock();
	QString name = metachunks.value(ohash);
	mutex.unlock();

	if (name.isEmpty())
		return QByteArray();	// Not present in index

	QString path = chunksprefix + name;
	QFile file(path);
	if (!file.open(QFile::ReadOnly)) {
		qDebug() << "Index: can't open metadata chunk file"
			<< path << "error" << file.errorString();
		bad:
		QMutexLocker lock(&mutex);
		if (metachunks.value(ohash) == name)
			metachunks.remove(ohash);
		QFile::remove(path);
		return QByteArray();
	}

	QByteArray data = file.readAll();
	if (data.isEmpty()) {
		qDebug() << "Index: can't read metadata chunk file"
			<< path << "error" << file.errorString();
		goto bad;
	}
	if (Sha512::hash(data) != ohash) {
		qDebug() << "Index: hash mismatch on metadata chunk file"
			<< path;
		goto bad;
	}

	return data;
}

void Index::fileMoved(const QString &, const QString &)
{
	qDebug() << "XXX Index::fileMoved";
}

QString Index::errorString()
{
	return indexerr;
}

QByteArray Index::readStore(const QByteArray &ohash)
{
	// Look for a metadata chunk first since it's already encoded
	QByteArray data = readMetaChunk(ohash);
	if (!data.isEmpty())
		return data;

	return readFileChunk(ohash);
}


