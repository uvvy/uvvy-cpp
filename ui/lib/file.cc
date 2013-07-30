
#include <QFileInfo>
#include <QDateTime>

#include "file.h"
#include "xdr.h"

using namespace SST;


////////// FileTreeStats //////////

QByteArray FileTreeStats::encode() const
{
	QByteArray buf;
	XdrStream ws(&buf, QIODevice::WriteOnly);
	ws << chunks << bytes;
	return buf;
}

FileTreeStats FileTreeStats::decode(const QByteArray &data)
{
	FileTreeStats ts;
	XdrStream rs(data);
	rs >> ts.chunks >> ts.bytes;
	if (rs.status() != rs.Ok)
		ts.chunks = ts.bytes = -1;
	return ts;
}


////////// FileInfo //////////

// XX this should probably just be in scan.cc;
// it's an awkward functionality split...
FileInfo::FileInfo(const QFileInfo &qfi)
:	t(Null)
{
	setName(qfi.fileName());
	setModTime(Time::fromQDateTime(qfi.lastModified()));
	// XX permissions, etc.

	// Set the type appropriately.
	// SymLink must be first since other predicates may succeed too
	// depending on what the symlink points to.
	if (qfi.isSymLink()) {
		t = SymLink;
		setLinkTarget(qfi.readLink());
	} else if (qfi.isDir()) {
		t = Directory;
	} else if (qfi.isFile()) {
		t = File;
	}
}

void FileInfo::setAttr(Attr tag, const QByteArray &value)
{
	if (value.isEmpty())
		at.remove(tag);
	else
		at.insert(tag, value);
}

bool FileInfo::operator==(const FileInfo &other) const
{
	if (t != other.t || at.size() != other.at.size())
		return false;

	foreach (Attr tag, attrs())
		if (attr(tag) != other.attr(tag))
			return false;

	return true;
}

void FileInfo::encode(XdrStream &ws) const
{
	ws << (qint32)t << (qint32)at.size();
	QHash<Attr, QByteArray>::const_iterator i = at.constBegin();
	while (i != at.constEnd()) {
		ws << (qint32)i.key() << i.value();
		++i;
	}
}

void FileInfo::decode(XdrStream &rs)
{
	// Produce an invalid FileInfo if something goes wrong
	t = Null;
	at.clear();

	// Decode the type code and attribute count
	qint32 typecode, nattrs;
	rs >> typecode >> nattrs;
	if (rs.status() != rs.Ok || nattrs < 0)
		return;

	// Read the attributes
	for (int i = 0; i < nattrs; i++) {
		qint32 tag;
		QByteArray attr;
		rs >> tag >> attr;
		if (rs.status() != rs.Ok)
			return;

		// Ignore attributes that exceed our maximums
		if (i >= maxAttrCount || attr.size() > maxAttrSize)
			continue;
		at.insert((Attr)tag, attr);
	}

	t = (Type)typecode;
}

QByteArray FileInfo::encode() const
{
	QByteArray data;
	XdrStream ws(&data, QIODevice::WriteOnly);
	ws << *this;
	return data;
}

FileInfo FileInfo::decode(const QByteArray &data)
{
	XdrStream rs(data);
	FileInfo fi;
	rs >> fi;
	return fi;
}


////////// AbstractDirReader //////////

void AbstractDirReader::readDir(const FileInfo &info)
{
	Q_ASSERT(info.isDirectory());

	// Decode the OpaqueInfo describing the directory metadata stream.
	OpaqueInfo ents = info.dirEntries();
	qint64 nrecs = ents.internalRecords();
	if (ents.isNull() || nrecs <= 0) {
		qDebug("AbstractDirReader: bad directory summary metadata");
		// XX record/indicate the error
		return readDone();
	}

	// Start asynchronously reading all the records in the directory.
	// We'll receive gotData() callbacks with the chunks.
	// We don't support multi-chunk records,
	// but directory entries should hopefully never be that big!
	readRecords(ents, 0, nrecs);
}

int AbstractDirReader::findPos(qint64 recno)
{
	// Binary search the recs array for the desired record number.
	unsigned lo = 0, hi = recs.size();
	while (hi > lo) {
		unsigned mid = (hi + lo) / 2;
		if (recs[mid] < recno)
			lo = mid+1;
		else
			hi = mid;
	}
	return lo;
}

void AbstractDirReader::gotData2(const QByteArray &, qint64, qint64 recno,
				const QByteArray &data, int nrecs)
{
	// Decode the directory entries in this block.
	XdrStream rs(data);
	QList<FileInfo> fil;
	for (int i = 0; i < nrecs; i++) {

		// Grab the next FileInfo representing a directory entry
		FileInfo fi;
		rs >> fi;
		if (rs.status() != rs.Ok) {
			qDebug("FileModel: bad directory entry metadata: "
				"record %d of %d", i, nrecs);
			// XX indicate the error properly somehow
			nrecs = i;
			break;
		}
		fil.append(fi);
	}
	if (!rs.atEnd())
		qDebug("FileModel: garbage at end of directory chunk");
	Q_ASSERT(fil.size() == nrecs);

	// Find the appropriate sorted list position for these entries.
	int pos = findPos(recno);
	if (pos < recs.size())
		Q_ASSERT(recs[pos] >= recno + nrecs);

	// Insert them into the record list
	for (int i = 0; i < nrecs; i++)
		recs.insert(pos+i, recno+i);

	// Pass the new entries to the client
	gotEntries(pos, recno, fil);
}

void AbstractDirReader::noData2(const QByteArray &, qint64, qint64 recno,
			qint64, int nrecs)
{
	// Find the position where the entries would go if we could get them.
	int pos = findPos(recno);

	// Pass the information on to the client.
	noEntries(pos, recno, nrecs);
}


////////// FileModel::Item //////////

FileModel::Item::Item(FileModel *model, Item *up,
			qint64 recno, const FileInfo &info)
:	AbstractDirReader(model), up(up), recno(recno), info(info),
	state(Unscanned)
{
}

QModelIndex FileModel::Item::index() const
{
	if (up == NULL)	{
		Q_ASSERT(this == model()->root);
		return QModelIndex();
	}

	int row = up->findPos(recno);
	Q_ASSERT(up->subs[row] == this);
	return model()->createIndex(row, 0, (void*)this);
}

void FileModel::Item::scan()
{
	Q_ASSERT(state == Unscanned);

	// If it's not a directory, nothing to do.
	if (!info.isDirectory()) {
		state = Scanned;
		return;
	}

	// Scan the directory.
	state = Scanning;
	readDir(info);
}

void FileModel::Item::gotEntries(int pos, qint64 recno,
				const QList<FileInfo> &fil)
{
	int nrecs = fil.size();
	Q_ASSERT(nrecs > 0);

	// Insert the new entries
	model()->beginInsertRows(index(), pos, pos + nrecs - 1);
	for (int i = 0; i < nrecs; i++)
		subs.insert(pos+i, new Item(model(), this, recno+i, fil[i]));
	model()->endInsertRows();
}

void FileModel::Item::noEntries(int, qint64 recno, int nrecs)
{
	// XXX display cleanly in the user interface
	qDebug("Couldn't find directory records %lld-%lld",
		recno, recno+nrecs-1);
}

void FileModel::Item::readDone()
{
	Q_ASSERT(state == Scanning);
	state = Scanned;
}


////////// FileModel //////////

FileModel::FileModel(QObject *parent, const QList<FileInfo> &files,
			Qt::ItemFlags itemFlags)
:	QAbstractItemModel(parent),
	itemFlags(itemFlags)
{
	root = new Item(this, NULL, 0, FileInfo());
	for (int i = 0; i < files.size(); i++)
		root->subs.append(new Item(this, root, i, files[i]));
}

int FileModel::rowCount(const QModelIndex &index) const
{
	Item *i = item(index);

	// If this item hasn't yet been scanned for subs, start scanning now.
	if (i->state == Item::Unscanned)
		i->scan();

	return i->subs.size();
}

int FileModel::columnCount(const QModelIndex &) const
{
	return columns;
}

QModelIndex FileModel::index(int row, int column,
				const QModelIndex &index) const
{
	Item *i = item(index);
	if (row < 0 || row >= i->subs.size())
		return QModelIndex();
	return createIndex(row, column, i->subs[row]);
}

QModelIndex FileModel::parent(const QModelIndex & index) const
{
	Item *i = (Item*)index.internalPointer();
	if (i == NULL || i->up == NULL)
		return QModelIndex();
	return i->up->index();
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation,
				int role) const
{
	if (orientation != Qt::Horizontal)
		return QVariant();

	switch (role) {
	case Qt::DisplayRole:
		switch (section) {
		case 0:		return tr("File Name");
		case 1:		return tr("Size");
		}
		break;
	case Qt::TextAlignmentRole:
		switch (section) {
		case 0:		return Qt::AlignLeft;
		case 1:		return Qt::AlignRight;
		}
		break;
	}
	return QVariant();
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
	Item *i = item(index);
	switch (role) {
	case Qt::DisplayRole:
		switch (index.column()) {
		case 0:		return i->info.name();
		case 1:		return i->info.fileSize();
		}
		break;
	case Qt::TextAlignmentRole:
		switch (index.column()) {
		case 0:		return Qt::AlignLeft;
		case 1:		return Qt::AlignRight;
		}
		break;
	}
	return QVariant();
}

Qt::ItemFlags FileModel::flags(const QModelIndex &) const
{
	return itemFlags;
}

