
#include <QtDebug>

#include "share.h"
#include "index.h"


static QHash<QString, ShareFile*> filehash;


////////// Share //////////

Share::Share(QObject *parent, const QString &root)
:	QObject(parent),
	root(root)
{
	insert(ShareFile::file(root, true));
}

Share::~Share()
{
	foreach (ShareFile *sf, files) {
		Q_ASSERT(sf->shares.contains(this));
		sf->shares.remove(this);
	}
	files.clear();
}

void Share::insert(ShareFile *sf, bool recursive)
{
	files.insert(sf);
	sf->shares.insert(this);

	if (recursive) {
		foreach (ShareFile *subsf, sf->subs.values()) {
			if (!exclude(subsf->path))
				insert(subsf);
		}
	}
}

void Share::remove(ShareFile *sf, bool recursive)
{
	if (!files.contains(sf)) {
		Q_ASSERT(!sf->shares.contains(this));
		return;
	}

	Q_ASSERT(sf->shares.contains(this));
	files.remove(sf);
	sf->shares.remove(this);

	if (recursive) {
		foreach (ShareFile *subsf, sf->subs.values())
			remove(subsf, true);
	}
}

bool Share::exclude(const QString &)
{
	return false;
}


////////// ShareFile //////////

ShareFile::ShareFile(const QString &path)
:	path(path)
{
	// Insert us into the global file table.
	Q_ASSERT(!filehash.contains(path));
	filehash.insert(path, this);

	// If we already have a parent, notify its Shares of a new child.
	checkparent();
}

ShareFile::~ShareFile()
{
	// Remove us from any Shares we are in
	foreach (Share *sh, shares)
		sh->remove(this);
	Q_ASSERT(shares.isEmpty());

	// Remove us from the global file table.
	Q_ASSERT(filehash.value(path) == this);
	filehash.remove(path);
}

QString ShareFile::fileName()
{
	int slashpos = path.lastIndexOf('/');
	if (slashpos < 0)
		return path;

	return path.mid(slashpos+1);
}

ShareFile *ShareFile::parent()
{
	int slashpos = path.lastIndexOf('/');
	if (slashpos < 0)
		return NULL;

	return file(path.mid(0, slashpos), false);
}

void ShareFile::update(const FileInfo &newInfo, const QDateTime newModTime)
{
	info = newInfo;
	modtime = newModTime;

	checkparent();
	updated();
}

void ShareFile::checkparent()
{
	ShareFile *par = parent();
	if (!par)
		return;

	QString name = fileName();
	ShareFile *child = par->subs.value(name);
	if (!child) {
		par->subs.insert(name, this);
		par->newChild(path);
	} else
		Q_ASSERT(child == this);
}

ShareFile *ShareFile::file(const QString &canonPath, bool create)
{
	ShareFile *sf = filehash.value(canonPath);
	if (sf || !create)
		return sf;

	sf = new ShareFile(canonPath);
	filehash.insert(canonPath, sf);
	return sf;
}


