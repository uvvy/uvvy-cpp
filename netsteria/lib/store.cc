
#include <QCache>

#include "store.h"


////////// Store //////////

QList<Store*> Store::stores;

// Cache size: enough for 10 max-size chunks
static QCache<QByteArray, QByteArray> chunkcache(10*1024*1024);

Store::Store()
{
	Q_ASSERT(!stores.contains(this));
	stores.append(this);
}

Store::~Store()
{
	int i = stores.indexOf(this);
	Q_ASSERT(i >= 0);
	stores.removeAt(i);
}

ChunkSearch *Store::searchStore(const QByteArray &, QObject *, const char *)
{
	return NULL;
}

QByteArray Store::readStores(const QByteArray &ohash)
{
	// Return a copy of the chunk from our cache, if available
	QByteArray *dp = chunkcache.object(ohash);
	if (dp != NULL)
		return *dp;

	// XX On success, bring succeeding store to the front?
	foreach (Store *st, stores) {
		QByteArray ba = st->readStore(ohash);
		if (!ba.isEmpty()) {
			chunkcache.insert(ohash, new QByteArray(ba), ba.size());
			return ba;
		}
	}
	return QByteArray();
}

ChunkSearch *Store::searchStores(const QByteArray &ohash,
			QObject *parent, const char *slot)
{
	StoresSearch *srch = new StoresSearch(parent);
	connect(srch, SIGNAL(relayFound(const QByteArray&)), parent, slot);

	foreach (Store *st, stores) {
		ChunkSearch *cs = st->searchStore(ohash, srch,
				SLOT(storeFound(const QByteArray&)));
		if (cs)
			srch->subs.append(cs);
	}

	return srch;
}


////////// StoresSearch //////////

bool StoresSearch::done()
{
	foreach (ChunkSearch *cs, subs)
		if (cs && !cs->done())
			return false;
	return true;
}

void StoresSearch::storeDone(const QByteArray &data)
{
	if (data.isEmpty()) {
		if (!done())
			return;		// Ignore all failures but last
	} else {
		if (done())
			return;		// Already returned data
	}

	// Schedule this search object and its sub-searches for deletion
	cancel();

	// Relay the results
	relayDone(data);
}

