//
// Classes for breaking opaque byte streams into encrypted chunks
// described by hierarchically structured metadata.
// XXX Just rename Opaque* to File*?
//
#ifndef OPAQUE_H
#define OPAQUE_H

#include <QList>
#include <QMultiHash>

#include "store.h"
#include "chunk.h"

class QIODevice;
class SST::XdrStream;
class OpaqueCoder;
class OpaqueReader;

#define KEYCACHEMAX 10  // Number of key-blocks reader should cache


/**
 * An OpaqueKey contains the information necessary to locate and decode
 * a particular contiguous portion of a logical opaque byte stream,
 * represented by either a data chunk or an indirect key chunk.
 */
struct OpaqueKey
{
    static const int xdrsize = 128; // Size of external representation

    qint32 level;       // 0 means key points directly to stream data
    qint32 osize;       // Outer size of immediate chunk key points to
    qint64 isize;       // Inner size of stream this key represents
    qint64 irecs;       // Numer of records starting in this region
    quint32 isum;       // Adler-32 checksum of inner stream content
    QByteArray key;     // 32-byte SHA-256 encryption key
    QByteArray ohash;   // 64-byte SHA-512 outer hash

    OpaqueKey();
    OpaqueKey(const OpaqueKey &other);

    inline void setNull() { level = -1; }
    inline bool isNull() const { return level < 0; }

    bool operator==(const OpaqueKey &other) const;
    inline bool operator!=(const OpaqueKey &other) const
        { return !(*this == other); }
};

SST::XdrStream &operator>>(SST::XdrStream &xs, OpaqueKey &k);
SST::XdrStream &operator<<(SST::XdrStream &xs, const OpaqueKey &k);

/**
 * An OpaqueInfo represents the top-level metadata about an opaque stream,
 * produced after scanning, chunking, and encoding an input stream.
 */
class OpaqueInfo
{
    friend class OpaqueCoder;

public:
    enum Type {
        Null = 0,   // Special invalid OpaqueInfo
        Inline1 = 1,    // Contains one record directly in-line
        InlineRecs = 2, // Contains two or more records in-line
        Indirect = 3,   // Contains keys indirectly pointing to data
    };

private:
    Type t;
    qint32 nrecs;
    QByteArray dat;
    QList<OpaqueKey> kl;


public:
    /**
     * Create a null (invalid) OpaqueInfo object with no metadata.
     */
    inline OpaqueInfo() : t(Null) { }
    OpaqueInfo(const OpaqueInfo &other);

    /**
     * Create an initialized direct (inline) or indirect OpaqueInfo
     * @param  inlineData [description]
     * @param  nrecords   [description]
     * @return            [description]
     */
    static OpaqueInfo direct(const QByteArray &inlineData,
                int nrecords = 1);
    static OpaqueInfo indirect(const QList<OpaqueKey> &indirectKeys);

    /**
     * Return type information
     */
    inline Type type() const { return t; }
    inline bool isNull() const { return t == Null; }
    inline bool isInline() const { return t == Inline1 || t == InlineRecs; }
    inline bool isIndirect() const { return t == Indirect; }

    /**
     * Return actual content
     */
    inline QByteArray inlineData() const { return dat; }
    inline QList<OpaqueKey> indirectKeys() const { return kl; }

    /**
     * Return the total internal size in bytes of the opaque stream.
     */
    qint64 internalSize() const;

    /**
     * Return the total number of logical records in the opaque stream.
     */
    qint64 internalRecords() const;

    /**
     * Return the internal Adler32 checksum of the entire opaque stream.
     */
    quint32 internalSum() const;

    /**
     * Serialize or de-serialize the OpaqueInfo.
     * (Can also use the <<, >> operators below with XdrStream.)
     */
    void encode(SST::XdrStream &ws) const;
    void decode(SST::XdrStream &ws);
    QByteArray encode() const;
    static OpaqueInfo decode(const QByteArray &data);

    /**
     * Compute max size of a serialized OpaqueInfo for given parameters
     * @param  dataSize [description]
     * @param  nrecords [description]
     * @return          [description]
     */
    static int directSize(int dataSize, int nrecords = 1);
    static int indirectSize(int nkeys);

};

inline SST::XdrStream &operator<<(SST::XdrStream &xs, const OpaqueInfo &oi)
    { oi.encode(xs); return xs; }
inline SST::XdrStream &operator>>(SST::XdrStream &xs, OpaqueInfo &oi)
    { oi.decode(xs); return xs; }

/**
 * Abstract base class for chunking and encoding opaque data streams.
 */
class OpaqueCoder : public QIODevice
{
    // Target minimum and maximum number of bytes in data chunks
    static const int minChunkSize = (256*1024);
    static const int maxChunkSize = (1024*1024);

    // Target minimum and maximum number of keys in key chunks
    //static const int minChunkKeys = minChunkSize / OpaqueKey::xdrsize;
    //static const int maxChunkKeys = maxChunkSize / OpaqueKey::xdrsize;
    static const int minChunkKeys = 4;  // XXX for testing only
    static const int maxChunkKeys = 16;


    // Cumulative statistics for already-written chunks
    qint64 totsize;         // Total size in bytes
    qint64 totrecs;         // Number of records written so far
    quint32 adleracc;       // Cumulative Adler32 sum of data
    quint32 adlerchk;       // Sanity check for above

    // Information about current chunk being built
    QByteArray buf;
    int avail;          // Number of bytes valid in buffer
    QList<int> marks;       // Record markers in this chunk

    QList<QList<OpaqueKey> > keys;

    QString err;


    // Convergently encrypt a (key or data) chunk,
    // producing ciphertext, decryption key (inner hash), and outer hash.
    static void encChunk(const void *data, int size, QByteArray &cyph,
            QByteArray &key, QByteArray &ohash);
    static void encChunk(const QByteArray &clear, QByteArray &cyph,
            QByteArray &key, QByteArray &ohash);

    bool doChunk();
    bool addKey(const OpaqueKey &key);


public:
    // Minimum allowed metadata limit
    static const int minMetaDataLimit = 4 + 4 + OpaqueKey::xdrsize;

    // Typical recommended metadata limit
    static const int typMetaDataLimit = 1024;


    OpaqueCoder(QObject *parent);

    // (Re-)implementations of QIODevice abstract methods
    virtual bool open(OpenMode mode);
    virtual bool isSequential() const;
    virtual qint64 readData (char *data, qint64 maxSize);
    virtual qint64 writeData (const char *data, qint64 maxSize);
    virtual qint64 pos() const;

    // Read data into this OpaqueCoder directly from another QIODevice.
    qint64 readFrom(QIODevice *in, qint64 maxSize = -1);

    /**
    Mark the beginning of a new record at the current position.
    Records can be any size, fixed or variable, up to 2^63-1 bytes.
    The coder ensures that each chunk begins on a record boundary,
    except for chunks that represent the continuation of
    large records of more than maxChunkSize bytes in size.
    It is possible to seek efficiently to the chunk containing
    (the beginning of) any desired record number in an opaque stream.
    The coder does NOT keep track of record boundaries
    within a chunk containing multiple small records.
    */
    void markRecord();

    /**
     * Return the number of complete records written so far -
     * i.e., the number of times markRecord() has been called.
     */
    inline qint64 records() { return totrecs + marks.size(); }

    /**
     * Finish coding and wrap everything up into a top-level OpaqueInfo
     * @param  metaDataLimit [description]
     * @return               [description]
     */
    OpaqueInfo finish(int metaDataLimit = typMetaDataLimit);

    /**
     * Read an entire stream from a QIODevice and then finish().
     * @param  in            [description]
     * @param  metaDataLimit [description]
     * @return               [description]
     */
    OpaqueInfo encode(QIODevice *in, int metaDataLimit = typMetaDataLimit);

    inline QString errorString() { return err; }
    inline void setErrorString(const QString &err) { this->err = err; }

    /**
     * Encode a data chunk, producing its ciphertext and OpaqueKey.
     * @param  data [description]
     * @param  size [description]
     * @param  recs [description]
     * @param  cyph [description]
     * @return      [description]
     */
    static OpaqueKey encDataChunk(const void *data, int size, qint64 recs,
                    QByteArray &cyph);
    static inline OpaqueKey encDataChunk(const QByteArray &data,
                    qint64 recs, QByteArray &cyph)
        { return encDataChunk(data.data(), data.size(), recs, cyph); }

    /**
     * Encode a key chunk, producing its ciphertext and OpaqueKey.
     * @param  keys [description]
     * @param  cyph [description]
     * @return      [description]
     */
    static OpaqueKey encKeyChunk(const QList<OpaqueKey> &keys,
                QByteArray &cyph);

protected:
    virtual ~OpaqueCoder();


    // Callback methods to give the client the chunks we produce
    virtual bool dataChunk(const QByteArray &chunk, const OpaqueKey &key,
                qint64 ofs, qint32 size) = 0;
    virtual bool keyChunk(const QByteArray &chunk, const OpaqueKey &key,
                const QList<OpaqueKey> &subkeys) = 0;
};


class AbstractOpaqueReader : public AbstractChunkReader
{
    Q_OBJECT

    // A sub-region of an opaque file to be read, defined by
    // a file offset and an OpaqueKey describing that region.
    struct Region
    {
        OpaqueInfo info;
        qint64 byteofs;     // Start of region - byte offset.
        qint64 recofs;      // Start of region - record number.
        OpaqueKey key;      // Key of chunk covering this region.
        qint64 start, end;  // Logical extent we actually want.
        bool byrec;     // 'start' and 'end' are record numbers
    };

    // Hash table of regions for which we need a data or key chunk,
    // indexed by the required chunk's outer hash.
    QMultiHash<QByteArray, Region> regions;


    static void decChunk(const void *data, int size, QByteArray &cyph,
            QByteArray &key, QByteArray &ohash);

protected:
    inline AbstractOpaqueReader(QObject *parent)
        : AbstractChunkReader(parent) { }

    // Add a region of a particular file to the reader's read-list,
    // and start trying to read the blocks covering that byte range.
    void readBytes(const OpaqueInfo &info, qint64 ofs, qint64 size);

    // Read all chunks comprising a particular range of record numbers.
    void readRecords(const OpaqueInfo &info, qint64 recofs, qint64 nrecs);

    // Add an entire file to the reader's read-list.
    inline void readAll(const OpaqueInfo &info)
        { readBytes(info, 0, info.internalSize()); }


    // Events that we signal to our derived class.
    // If the file is inline, gotData() receives an empty ohash.
    virtual void gotData(const QByteArray &ohash, qint64 ofs, qint64 recno,
                const QByteArray &data, int nrecs) = 0;
    virtual void noData(const QByteArray &ohash, qint64 ofs, qint64 recno,
                qint64 size, int nrecs) = 0;
    virtual void readDone();

    // Our subclass can optionally override this method
    // in order to capture (and save, for example) the metadata chunks
    // we obtain and use during the process of reading the opaque data.
    // The ofs/recno/size/nrecs indicates the region of the opaque file
    // that this particular metadata chunk pertains to.
    virtual void gotMetaData(const QByteArray &ohash,
                const QByteArray &chunk,
                qint64 ofs, qint64 recno,
                qint64 size, int nrecs);

    // Our implementations of AbstractChunkReader's callback methods
    virtual void gotData(const QByteArray &ohash, const QByteArray &data);
    virtual void noData(const QByteArray &ohash);

private:
    void readRegion(const OpaqueInfo &info,
            qint64 start, qint64 end, bool byrec);

    void addKeys(const Region &r, const QList<OpaqueKey> &keys);
};

class OpaqueReader : public AbstractOpaqueReader
{
    Q_OBJECT

public:
    inline OpaqueReader(QObject *parent)
        : AbstractOpaqueReader(parent) { }

signals:
    void gotData(const OpaqueInfo &info, qint64 ofs, qint64 recno,
                const QByteArray &data, int nrecs);
    void noData(const OpaqueInfo &info, qint64 ofs, qint64 recno,
                qint64 size, int nrecs);
    void readDone();
};


#if 0
// Reads complete records from an opaque data stream,
// automatically merging large multi-chunk records into one QByteArray.
class AbstractRecordReader : public AbstractOpaqueReader
{
    ...
};
#endif


#if 0
class OpaqueFile : public QObject
{
    // Pointer to parent train of next higher level.
    // If NULL, our level is variable and can increase on demand.
    Train *parent;

    // Level of this train:
    // level 0 contains pointers directly to data chunks.
    int level;

    QList<Train*> subs;
    QList<TrainKey> keys;

public:
    Train(QObject *parent = NULL);

    static OpaqueFile *create(QIODevice *strm);

private:
    Train(Train *parent, int level);

    Train *appendKey(const TrainKey &tk);
};

class OpaqueDataChunk : public FileChunk
{
};

class OpaqueKeyChunk : public FileChunk
{
};
#endif

#endif  // OPAQUE_H
