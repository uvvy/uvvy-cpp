#pragma once

#include <QObject>
#include <QPointer>
#include <QList>

class QByteArray;

/**
 * A ChunkSearch represents an in-progress asynchronous search for a chunk.
 */
struct ChunkSearch : public QObject
{
    inline ChunkSearch(QObject *parent) : QObject(parent) { }

    inline void cancel() { deleteLater(); }

    virtual bool done() = 0;
};

/**
 * Abstract base class for objects providing chunk storage and lookup.
 * A Store is an abstract repository from which chunks can be looked up.
 * This module keeps a systemwide list of all instantiated Store objects,
 * and provides static methods to search all of them for a desired chunk.
 */
class Store : public QObject
{
    static QList<Store*> stores;

public:
    Store();
    virtual ~Store();

    // Read the contents of a given chunk, if it's locally available.
    // Returns an empty QByteArray if not immediately available.
    virtual QByteArray readStore(const QByteArray &ohash) = 0;

    // Initiate a request to find a given chunk, remotely if necessary.
    // Signals the specified slot of the specified parent object
    // with the chunk contents (QByteArray) are found or the search fails.
    // The resulting ChunkSearch also becomes of a child of 'parent',
    // thus canceling the search if 'parent' is deleted.
    // The default implementation returns NULL,
    // indicating this store doesn't support asynchronous lookup.
    virtual ChunkSearch *searchStore(const QByteArray &ohash,
                QObject *parent, const char *slot);

    // Write a chunk into this store.
    //virtual bool writeChunk(const QByteArray &data) = 0;


    ///// Static methods /////

    // Search through all stores for a locally-available chunk.
    static QByteArray readStores(const QByteArray &ohash);

    // Initiate an asynchronous request for a chunk,
    // searching all available stores in parallel.
    static ChunkSearch *searchStores(const QByteArray &ohash,
                QObject *receiver, const char *slot);
};

#if 0
// A UnionStore represents the logical union of one or more sub-Stores.
class UnionStore : public Store
{
    virtual QByteArray readStore(const QByteArray &ohash);

    virtual ChunkSearch *searchStore(const QByteArray &ohash,
                QObject *parent, const char *slot);


public:
};
#endif

/**
 * Private helper class for Store
 */
class StoresSearch : public ChunkSearch
{
    friend class Store;
    Q_OBJECT

    QList<QPointer<ChunkSearch> > subs;


    inline StoresSearch(QObject *parent) : ChunkSearch(parent) { }

    virtual bool done();

private slots:
    void storeDone(const QByteArray &data);

signals:
    void relayDone(const QByteArray &data);
};
