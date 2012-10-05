//
// File and directory metadata management
#ifndef FILE_H
#define FILE_H

#include <QHash>
#include <QAbstractItemModel>

#include "timer.h"
#include "opaque.h"
#include "util.h"

class SST::XdrStream;
class QFileInfo;


struct FileTreeStats
{
	// Total number of indirect data and metadata chunks in tree
	qint64	chunks;

	// Total number of bytes in all chunks representing tree
	qint64	bytes;


	inline FileTreeStats()
		: chunks(-1), bytes(-1) { }
	inline FileTreeStats(const FileTreeStats &other)
		: chunks(other.chunks), bytes(other.bytes) { }

	inline bool isNull()
		{ return chunks < 0 || bytes < 0; }

	QByteArray encode() const;
	static FileTreeStats decode(const QByteArray &data);
};


// FileInfo represents a Netsteria file metadata record.
// The contents of a directory is a sequence of encoded FileInfo records.
class FileInfo
{
public:
	// Maximum size and number of attributes we're willing to read
	static const int maxAttrSize = 65536;
	static const int maxAttrCount = 256;

	// Fundamental file type codes
	enum Type {
		Null		= 0x0000,	// Special null FileInfo
		File		= 0x0001,	// Flat byte-oriented file
		Directory	= 0x0002,	// Directory with sub-files
		SymLink		= 0x0003,	// Symbolic link
		UnixDev		= 0x0004,	// Unix device special file
	};

	// File attribute tag codes.
	// The upper 16 bits are for attribute property flags.
	enum AttrFlag {
		Invalid		= 0x00000000,	// Invalid attribute tag

		// Flags indicating useful properties of certain tags
		DirectFlag	= 0x00010000,	// Attr data is direct
		OpaqueFlag	= 0x00020000,	// Attr data is 1-level opaque
		ContentFlag	= 0x00040000,	// Primary file content
		RobustFlag	= 0x00080000,	// Robust against file changes

		// Tags representing basic file content
		FileData	= 0x00060001,	// Contents of flat file
		DirEntries	= 0x00040002,	// Contents of directory
		LinkTarget	= 0x00050003,	// UTF-8 symbolic link target
		UnixDevInfo	= 0x00050004,	// Unix device file info

		// Tags representing general file attributes
		Name		= 0x00090101,	// UTF-8 filename
		Comment		= 0x00090102,	// UTF-8 human-readable comment
		ModTime		= 0x00010103,	// Time of last modification
		MimeType	= 0x00010104,	// MIME content type (ASCII)
		UnixStats	= 0x00010105,	// Unix file statistics
		TreeStats	= 0x00010106,	// Recursive content statistics

		// Tags representing file hashes of different types
		MD5		= 0x00010201,	// MD5 hash of file contents
		SHA1		= 0x00010202,	// SHA-1 hash of file
		SHA256		= 0x00010203,	// SHA-256 hash of file
		SHA384		= 0x00010204,	// SHA-384 hash of file
		SHA512		= 0x00010205,	// SHA-512 hash of file
	};
	Q_DECLARE_FLAGS(Attr, AttrFlag)

private:
	Type t;
	QHash<Attr, QByteArray> at;


public:
	inline FileInfo() : t(Null) { }

	// Create a FileInfo initialized from the metadata in a QFileInfo
	FileInfo(const QFileInfo &qfi);

	inline Type type() const { return t; }
	inline void setType(Type t) { this->t = t; }

	// Convenient tests for specific types
	inline bool isNull() const { return t == Null; }
	inline bool isFile() const { return t == File; }
	inline bool isDirectory() const { return t == Directory; }
	inline bool isSymLink() const { return t == SymLink; }

	// Raw attribute get/set methods
	inline QByteArray attr(Attr tag) const { return at.value(tag); }
	void setAttr(Attr tag, const QByteArray &value);
	inline void removeAttr(Attr tag) { setAttr(tag, QByteArray()); }
	inline QList<Attr> attrs() const { return at.keys(); }

	// Type-specific methods for file content attributes
	inline OpaqueInfo fileData() const
		{ return OpaqueInfo::decode(attr(FileData)); }
	inline qint64 fileSize() const
		{ return fileData().internalSize(); }
	inline OpaqueInfo dirEntries() const
		{ return OpaqueInfo::decode(attr(DirEntries)); }
	inline QString linkTarget() const
		{ return QString::fromUtf8(attr(LinkTarget)); }

	inline void setFileData(const OpaqueInfo &info)
		{ setAttr(FileData, info.encode()); }
	inline void setDirEntries(const OpaqueInfo &info)
		{ setAttr(DirEntries, info.encode()); }
	inline void setLinkTarget(const QString &target)
		{ setAttr(LinkTarget, target.toUtf8()); }

	// Type-specific get/set methods for general file attributes
	inline QString name() const
		{ return QString::fromUtf8(attr(Name)); }
	inline QString comment() const
		{ return QString::fromUtf8(attr(Comment)); }
	inline SST::Time modTime() const
		{ return SST::Time::decode(attr(ModTime)); }
	inline QString mimeType() const
		{ return QString::fromAscii(attr(MimeType)); }
	inline FileTreeStats treeStats() const
		{ return FileTreeStats::decode(attr(TreeStats)); }

	inline void setName(const QString &name)
		{ setAttr(Name, name.toUtf8()); }
	inline void setComment(const QString &comment)
		{ setAttr(Comment, comment.toUtf8()); }
	inline void setModTime(const SST::Time &time)
		{ setAttr(ModTime, time.encode()); }
	inline void setMimeType(const QString &mimeType)
		{ setAttr(Comment, mimeType.toAscii()); }
	inline void setTreeStats(const FileTreeStats &st)
		{ setAttr(TreeStats, st.encode()); }

	// Info block comparison
	bool operator==(const FileInfo &other) const;
	inline bool operator!=(const FileInfo &other) const
		{ return !(*this == other); }

	// Serialization of file metadata
	void encode(SST::XdrStream &ws) const;
	void decode(SST::XdrStream &ws);
	QByteArray encode() const;
	static FileInfo decode(const QByteArray &data);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FileInfo::Attr)

inline SST::XdrStream &operator<<(SST::XdrStream &xs, const FileInfo &fi)
	{ fi.encode(xs); return xs; }
inline SST::XdrStream &operator>>(SST::XdrStream &xs, FileInfo &fi)
	{ fi.decode(xs); return xs; }


// This class provides convenient asynchronous directory reading.
// When it reads a directory chunk, it decodes the chunk into a FileInfo list,
// and then supplies the list along with the position at which the new entries
// should be inserted in a list the client is presumably building.
class AbstractDirReader : public AbstractOpaqueReader
{
public:
private:
	QList<qint64> recs;

public:
	inline AbstractDirReader(QObject *parent)
		: AbstractOpaqueReader(parent) { }

	// Scan the contents of the given directory
	void readDir(const FileInfo &dirInfo);

	// Find the position in the sorted array of already-loaded records
	// where a particular record number is or should go.
	int findPos(qint64 recno);

	// Callback functions the subclass must provide
	virtual void gotEntries(int pos, qint64 recno,
				const QList<FileInfo> &ents) = 0;
	virtual void noEntries(int pos, qint64 recno, int nents) = 0;

protected:

	// Our implementations of AbstractOpaqueReader's abstract methods
	void gotData(const QByteArray &ohash,
				qint64 ofs, qint64 recno,
				const QByteArray &data, int nrecs);
	void noData(const QByteArray &ohash,
				qint64 ofs, qint64 recno,
				qint64 size, int nrecs);
};

// Concrete version of AbstractDirReader that sends signals.
class DirReader : public AbstractDirReader
{
public:
	inline DirReader(QObject *parent)
		: AbstractDirReader(parent) { }

signals:
	void gotEntries(int pos, qint64 recno, const QList<FileInfo> &ents);
	void noEntries(int pos, qint64 recno, int nents);
	void readDone();
};


// Netsteria File browser model that can handle multiple top-level files
// and browsing into subdirectories.
class FileModel : public QAbstractItemModel
{
	static const int columns = 2;

	struct Item : public AbstractDirReader {
		Item *up;
		qint64 recno;
		FileInfo info;
		QList<Item*> subs;	// Sub-items, sorted by recno

		enum State {
			Unscanned,	// Haven't started reading directory
			Scanning,	// Currently reading directory
			Scanned,	// Finished reading directory
		} state;

		Item(FileModel *model, Item *up,
				qint64 recno, const FileInfo &info);

		// A FileModel::Item is always a child of its FileModel.
		inline FileModel *model() const
			{ return (FileModel*)parent(); }

		// Create a QModelIndex pointing to this item.
		QModelIndex index() const;

		// Start scanning for any sub-items that might be present.
		// This process will happen asynchronously.
		void scan();

		virtual void gotEntries(int pos, qint64 recno,
					const QList<FileInfo> &ents);
		virtual void noEntries(int pos, qint64 recno, int nents);
		virtual void readDone();
	};
	Item *root;

	Qt::ItemFlags itemFlags;

	inline Item *item(const QModelIndex &index) const {
		Item *i = (Item*)index.internalPointer();
		return i ? i : root; }

	virtual int rowCount(
			const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(
			const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex index(int row, int column,
			const QModelIndex & parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex & index) const;
	virtual QVariant headerData(int section, Qt::Orientation orient,
			int role = Qt::DisplayRole) const;
	virtual QVariant data(const QModelIndex &index,
			int role = Qt::DisplayRole) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

public:
	FileModel(QObject *parent, const QList<FileInfo> &files,
		Qt::ItemFlags itemFlags = Qt::ItemIsEnabled
					| Qt::ItemIsSelectable);
};

#endif	// FILE_H
