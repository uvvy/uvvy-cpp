
#include <QQueue>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QLocale>
#include <QtDebug>

#include "scan.h"
#include "share.h"
#include "index.h"

using namespace SST;


static Scanner *scanner;

#define scanmutex scanner->mutex
#define scancond scanner->cond

static QQueue<ScanItem*> scanqueue;


////////// Scan //////////

Scan::Scan(QObject *parent, const QString &origpath, Options opts)
:	Action(parent, tr("Scan %0").arg(origpath)),
	path(origpath), opts(opts),
	qfi(origpath),
	nfiles(1), nerrors(0),
	gotbytes(0), totbytes(0),
	item(NULL)
{
	// Canonicalize the file path.
	// If the path doesn't exist, just use the absolute path.
	QString path = qfi.canonicalFilePath();
	if (path.isEmpty())
		path = qfi.absoluteFilePath();
	Q_ASSERT(!path.isEmpty());
}

Scan::~Scan()
{
	if (item) {
		Q_ASSERT(item->parent() == this);
		QMutexLocker lock(&scanmutex);
		if (!item->done) {
			// Mark it cancelled and detach -
			// the scanner thread will delete it when done.
			item->cancel = true;
			item->setParent(NULL);
		}
	}
}

void Scan::start()
{
	if (isRunning())
		return;

	qDebug() << "Scan:" << path;

	// Find or create the ShareFile for this path
	ShareFile *sf = ShareFile::file(path, true);

	// If it doesn't exist, just mark the file inaccessible and we're done.
	if (!qfi.exists()) {
		qDebug() << "Can't access file" << path;
		FileInfo info;
		info.setComment(tr("Inaccessible file"));
		sf->update(info, QDateTime::currentDateTime());
		return setError(tr("Can't access file"));
	}

	// If it's not a directory and hasn't changed since last scan,
	// then we probably don't need to re-scan it -
	// just use the old metadata from our local file index.
	info = Index::getFileInfo(path);
#if 0
	qDebug() << "retrieved fileinfo - name" << info.name()
		<< "modtime" << info.modTime().toString();
	qDebug() << "a" << Time::fromQDateTime(qfi.lastModified()).usecs
		<< "b" << info.modTime().usecs;
	if (opts & ForceRescan)
		qDebug() << "ForceRescan";
	if (info.isNull())
		qDebug() << "isnull";
#endif
	if (!(opts & ForceRescan) && !qfi.isDir() && !info.isNull()
			&& (Time::fromQDateTime(qfi.lastModified())
				== info.modTime()))
		return reportDone();

	// Start building a new FileInfo metadata block
	info = FileInfo(qfi);

	// Process the object according to its type.
	// SymLink must be first since other predicates may succeed too
	// depending on what the symlink points to.
	switch (info.type()) {
	case FileInfo::File:
		scanFile(qfi);
		break;
	case FileInfo::Directory:
		scanDir(qfi);
		break;
	default:
		// XXX symlinks
		return setError(tr("Unknown file system object"));
	}
}

void Scan::scanFile(const QFileInfo &qfi)
{
	if (!scanner) {
		scanner = new Scanner();
		scanner->start();
	}

	totbytes = qfi.size();
	qDebug() << "Scan: file size" << totbytes;

	// Build a work item for the scanner thread
	Q_ASSERT(item == NULL);
	item = new ScanItem(this);
	connect(item, SIGNAL(progress()), this, SLOT(fileProgress()));

	// Ship the file off to our dedicated scanner thread.
	QMutexLocker lock(&scanmutex);
	scanqueue.enqueue(item);
	scancond.wakeOne();

	report();
}

void Scan::fileProgress()
{
	if (!item)
		return;
	Q_ASSERT(isRunning());

	scanmutex.lock();
	bool done = item->done;
	gotbytes = item->gotbytes;
	scanmutex.unlock();

	// file may grow while scanning
	totbytes = qMax(gotbytes, totbytes);

	// If the scanner thread isn't done yet, just report progress.
	if (!done)
		return report();

	totbytes = gotbytes;			// final file size
	info.setFileData(item->datainfo);	// final file content info
	QString errmsg = item->errmsg;

	// We're done with the work item object
	delete item;
	item = NULL;

	// Check for scanning errors
	if (!errmsg.isEmpty()) {
		qDebug() << "Error scanning:" << errmsg;
		info.setComment(errmsg);
		return setError(errmsg);
	}

	reportDone();
}

void Scan::scanDir(const QFileInfo &)
{
	deleteAllParts();

	// Scan the directory
	QStringList ents = QDir(path).entryList(
			QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot |
				QDir::Hidden | QDir::System,
			QDir::Name);
	int nents = ents.size();

	// Recursively (re-)scan the directory entries,
	// building a new table of sub-Shares in the process.
	if (nents > 0) {
		beforeInsertParts(0, nents-1);
		for (int i = 0; i < nents; i++) {
			const QString &name = ents[i];
			Scan *sub = new Scan(this, path + "/" + name, opts);

			// Monitor our subs using queued signals,
			// so that back-propagation is as lazy as possible.
			connect(sub, SIGNAL(statusChanged()),
				this, SLOT(dirProgress()),
				Qt::QueuedConnection);

			appendPart(sub, false);

			// Start the sub-scan lazily via its retrytimer,
			// so that the recursion happens through the event loop.
			sub->startRetryTimer(0);
		}
		afterInsertParts(0, nents-1);
	}

	report();
}

void Scan::dirProgress()
{
	if (!isRunning())
		return;

	// Update our summary information for this directory
	bool subsdone = true;
	int newfiles = 1, newerrors = 0;
	qint64 newgotbytes = 0, newtotbytes = 0;
	for (int i = 0; i < numParts(); i++) {
		Scan *sub = (Scan*)part(i);

		newfiles += sub->nfiles;
		newerrors += sub->nerrors;
		newgotbytes += sub->gotbytes;
		newtotbytes += sub->totbytes;
		if (!sub->isDone())
			subsdone = false;
	}

	// Report progress if it's changed
	if (nfiles != newfiles || nerrors != newerrors ||
			gotbytes != newgotbytes || totbytes != newtotbytes) {
		nfiles = newfiles;
		nerrors = newerrors;
		gotbytes = newgotbytes;
		totbytes = newtotbytes;
		report();
	}

	// We're done here unless we've finished scanning all subs.
	if (!subsdone)
		return;

	// Build the directory stream metadata.
	ScanCoder coder(NULL, NULL);
	for (int i = 0; i < numParts(); i++) {
		Scan *sub = (Scan*)part(i);
		coder.write(sub->info.encode());
		coder.markRecord();
	}
	OpaqueInfo entinfo = coder.finish();
	if (entinfo.isNull()) {
		qDebug() << "Error coding directory:" << coder.errorString();
		return setError(tr("Error coding directory: %0")
				.arg(coder.errorString()));
	}
	info.setDirEntries(entinfo);

	reportDone();
}

void Scan::report()
{
	QString text = tr("Scanning: %0 of %1 bytes in %2 files")
			.arg(QLocale::system().toString(gotbytes))
			.arg(QLocale::system().toString(totbytes))
			.arg(QLocale::system().toString(nfiles));
	if (nerrors)
		text += tr(" (%1 errors detected)").arg(nerrors);
	setStatus(Running, text);

	if (totbytes > 0)
		setProgress((float)gotbytes / (float)totbytes);
	else
		setProgress(1.0);
}

void Scan::reportDone()
{
	// Keep the latest scanned file info in our file index
	Index::addFileInfo(path, info);

	QString text = tr("Scanned: %0 bytes in %1 files")
			.arg(totbytes).arg(nfiles);
	if (nerrors)
		text += tr(" (%1 errors detected)").arg(nerrors);
	setStatus(Success, text);
}


////////// ScanItem //////////

ScanItem::ScanItem(Scan *scan)
:	QObject(scan),
	path(scan->filePath()),
	gotbytes(0),
	done(false), cancel(false)
{
}


////////// Scanner //////////

Scanner::Scanner()
{
}

// Scanner thread main loop.
void Scanner::run()
{
	qDebug() << "Scanner thread running";

	forever {
		ScanItem *item = getwork();
		scanFile(item);
	}
}

// Get file scanning work - runs in the context of a separate scanner thread.
ScanItem *Scanner::getwork()
{
	QMutexLocker lock(&scanmutex);
	forever {
		if (!scanqueue.isEmpty())
			return scanqueue.dequeue();

		// Wait for some work
		scancond.wait(&scanmutex);
	}
}

// Scan a file - runs in the context of a separate scanner thread.
void Scanner::scanFile(ScanItem *item)
{
	QFile f(item->path);
	if (!f.open(f.ReadOnly)) {
		qDebug() << "Can't open file" << item->path;
		QMutexLocker lock(&scanmutex);
		item->errmsg = tr("Inaccessible file: %0")
				.arg(f.errorString());
		item->done = true;
		return item->progress();
	}

	// Scan the file contents
	ScanCoder coder(NULL, item);
	item->datainfo = coder.encode(&f);
	if (item->datainfo.isNull())
		item->errmsg = coder.errorString();

	QMutexLocker lock(&scanmutex);

	// Just delete the ScanItem if it's been orphaned.
	if (item->cancel)
		return delete item;

	// Send a final progress notification back to the main thread.
	item->gotbytes = item->datainfo.internalSize();
	item->done = true;
	item->progress();
}


////////// ScanCoder //////////

// Data chunk callback - runs in the context of scanner thread.
bool ScanCoder::dataChunk(const QByteArray &chunk, const OpaqueKey &key,
		qint64 ofs, qint32 size)
{
//	qDebug("File %s: data chunk - offset %lld, size %d, hash %s",
//		sh->path.toLocal8Bit().data(), ofs, size,
//		key.ohash.toBase64().data());

	if (item) {
		if (!Index::addFileChunk(key.ohash, item->path, ofs, size))
			return false;	// XXX handle errors properly

		// Send a progress report back to the main thread
		QMutexLocker lock(&scanmutex);
		Q_ASSERT(!item->done);
		item->gotbytes = ofs;
		if (item->cancel)
			return false;
		item->progress();

		return true;
	} else
		return Index::addMetaChunk(key.ohash, chunk);
}

// Key chunk callback - runs in the context of scanner thread.
bool ScanCoder::keyChunk(const QByteArray &chunk, const OpaqueKey &key,
		const QList<OpaqueKey> &)
{
//	qDebug("File %s: key chunk - %d keys, hash %s",
//		sh->path.toLocal8Bit().data(), subkeys.size(),
//		key.ohash.toBase64().data());

	return Index::addMetaChunk(key.ohash, chunk);
}


