
#include <QDir>
#include <QLocale>
#include <QtDebug>

#include "update.h"
#include "index.h"


////////// Update //////////

Update::Update(QObject *parent, const FileInfo &info, const QString &path)
:	Action(parent, tr("Download %0").arg(info.name())),
	info(info), path(path),
	totbytes(0), gotbytes(0), nerrors(0),
	fr(NULL), dr(NULL)
{
}

void Update::start()
{
	qDebug() << "Update: download file" << info.name()
		<< "to" << path << "attrs" << info.attrs().size();

	if (info.isFile()) {
		if (!fr)
			fr = new FileReader(this);
		fr->start();

	} else if (info.isDirectory()) {
		if (!dr)
			dr = new DirReader(this);
		dr->start();

	} else {
		setStatus(FatalError, tr("Unknown file system object type"));
	}
}

void Update::report()
{
	QString text = tr("Downloading: %0 of %1 bytes")
			.arg(QLocale::system().toString(gotbytes))
			.arg(QLocale::system().toString(totbytes));
	if (nerrors)
		text = tr("%0 (%1 errors detected)").arg(text).arg(nerrors);
	setStatus(Running, text);

	if (totbytes > 0)
		setProgress((float)gotbytes / (float)totbytes);
	else
		setProgress(1.0);
}

void Update::subChanged()
{
	Q_ASSERT(dr);
	dr->check();
}


////////// Update::FileReader //////////

Update::FileReader::FileReader(Update *up)
:	AbstractOpaqueReader(up),
	up(up),
	tmp(up->path + ".tmp")
{
	up->totbytes = up->info.fileSize();
}

Update::FileReader::~FileReader()
{
	// XXX remove the temporary download file from the Index's index
}

void Update::FileReader::start()
{
	if (up->isRunning())
		return;

	// If the file is empty, there's nothing to download...
	if (up->totbytes == 0) {
		QFile f(up->path);
		if (!f.open(QFile::WriteOnly | QFile::Truncate))
			return error(f.errorString());
		up->setStatus(up->Success,
				tr("Download successful (empty file)"));
		return;
	}

	// We wait to open the download file until we get the first chunk.
	// That way we don't hold file handles open for small files.
	up->gotbytes = 0;
	up->report();		// Sets us into the Running state
	readAll(up->info.fileData());
}

void Update::FileReader::error(const QString &msg)
{
	qDebug() << "FileReader: error" << msg;

	// Close our temporary file when we detect an error condition -
	// partly in hopes that closing and reopening it might help,
	// partly so we don't hold a file handle open until restart time.
	if (tmp.isOpen())
		tmp.close();

	up->setError(msg);
}

void Update::FileReader::gotData(const QByteArray &ohash, qint64 ofs, qint64,
				const QByteArray &data, int)
{
	if (!up->isRunning())
		return;

//	qDebug() << "Got file" << sh->filePath() << "offset" << ofs
//		<< "size" << data.size() << "to" << tmp.fileName();

	// Open the temporary download file
	if (!tmp.isOpen() && !tmp.open())
		return error(tr("Cannot create: %0")
				.arg(tmp.errorString()));

	if (!tmp.seek(ofs))
		return error(tr("Cannot seek: %0").arg(tmp.errorString()));

	if (tmp.write(data.data(), data.size()) != data.size())
		return error(tr("Cannot write: %0").arg(tmp.errorString()));

	// Index the data we're putting in the temporary file, for restart
	if (!ohash.isEmpty())
		Index::addFileChunk(ohash, tmp.fileName(),
					ofs, data.size());

	up->gotbytes += data.size();
	if (up->gotbytes > up->totbytes) {
		qDebug("FileReader: got more data than the file contains!");
		up->gotbytes = up->totbytes;
	}
	up->report();
}

void Update::FileReader::gotMetaData(const QByteArray &ohash,
				const QByteArray &cyph,
				qint64, qint64, qint64, int)
{
	Index::addMetaChunk(ohash, cyph);
}

void Update::FileReader::noData(const QByteArray &, qint64, qint64,
				qint64, int)
{
	error("Can't find file data - source may not be online");
}

void Update::FileReader::readDone()
{
	if (!up->isRunning())
		return;

	Q_ASSERT(up->gotbytes == up->totbytes);
	Q_ASSERT(tmp.isOpen());

	QString tmpPath = tmp.fileName();
	tmp.setAutoRemove(false);
	tmp.close();
	QFile::remove(up->path);
	if (!QFile::rename(tmpPath, up->path))
		return error(tr("Error finalizing downloaded file"));

	qDebug() << "Wrote:" << up->path;
	up->setStatus(up->Success,
			tr("Download successful (%0 bytes)")
			.arg(QLocale::system().toString(up->totbytes)));
}


////////// Update::DirReader //////////

Update::DirReader::DirReader(Update *up)
:	AbstractDirReader(up),
	up(up),
	dirdone(false)
{
}

void Update::DirReader::start()
{
	if (up->isRunning())
		return;

	QDir().mkdir(up->path);

	readDir(up->info);
}

void Update::DirReader::gotData(const QByteArray &ohash, const QByteArray &cyph)
{
	// Save the metadata chunk for quick restart.
	Index::addMetaChunk(ohash, cyph);

	// Pass the chunk on to AbstractDirReader.
	AbstractOpaqueReader::gotData(ohash, cyph);
}

void Update::DirReader::gotEntries(int pos, qint64, const QList<FileInfo> &ents)
{
	qDebug() << "DirReader: got" << ents.size() << "entries";

	int nents = ents.size();
	Q_ASSERT(nents > 0);
	Q_ASSERT(pos >= 0 && pos <= up->numParts());
	up->beforeInsertParts(pos, pos + nents - 1);

	for (int i = 0; i < nents; i++) {
		const FileInfo &subinfo = ents[i];
		const QString &name = subinfo.name();
		qDebug() << "DirReader:" << name << "at" << (pos+i);

		Update *sub = new Update(up, subinfo, up->path + "/" + name);
		up->insertPart(pos+i, sub, false);

		connect(sub, SIGNAL(statusChanged()),
			up, SLOT(subChanged()));
		connect(sub, SIGNAL(progressChanged(float)),
			up, SLOT(subChanged()));
	}

	up->afterInsertParts(pos, pos + nents - 1);

	for (int i = 0; i < nents; i++)
		up->part(pos+i)->start();
}

void Update::DirReader::noEntries(int, qint64, int)
{
	up->setError("Can't find directory data - source may not be online");
}

void Update::DirReader::readDone()
{
	qDebug() << "Finished scanning directory" << up->path;

	// We're done scanning the directory, but need to wait for our subs.
	dirdone = true;
	check();
}

void Update::DirReader::check()
{
	if (up->isDone())
		return;

	qint64 newtot = 0, newgot = 0;
	int newerrs = 0;
	bool subsdone = true;
	for (int i = 0; i < up->numParts(); i++) {
		Update *sub = (Update*)up->part(i);

		newtot += sub->totbytes;
		newgot += sub->gotbytes;

		newerrs += sub->nerrors;
		if (sub->isBlocked())
			newerrs++;

		if (!sub->isDone())
			subsdone = false;
	}

	if (up->totbytes != newtot || up->gotbytes != newgot ||
			up->nerrors != newerrs) {
		up->totbytes = newtot;
		up->gotbytes = newgot;
		up->nerrors = newerrs;
		up->report();
	}

	// XXX report fatal errors in subs
	if (dirdone && subsdone) {
		if (up->nerrors)
			up->setStatus(up->FatalError,
				tr("Download failed (%0 errors)")
				.arg(up->nerrors));
		else
			up->setStatus(up->Success,
				tr("Download Successful (%0 bytes)")
				.arg(up->totbytes));
	}
}


