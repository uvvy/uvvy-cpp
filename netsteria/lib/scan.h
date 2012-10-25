#pragma once

#include <QFileInfo>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "action.h"
#include "opaque.h"
#include "file.h"

class ScanItem;
class Scanner;

/**
 * Asynchronous directory and file scanning.
 */
class Scan : public Action
{
    Q_OBJECT

public:
    enum Option {
        NoOptions = 0x00,
        ForceRescan = 0x01, // Rescan even if apparently unmodified
    };
    Q_DECLARE_FLAGS(Options, Option)

private:
    QString path;
    Options opts;
    QFileInfo qfi;
    FileInfo info;
    int nfiles, nerrors;
    qint64 gotbytes, totbytes;
    ScanItem *item;

public:
    Scan(QObject *parent, const QString &path, Options opts = NoOptions);
    ~Scan();
    virtual void start();

    inline QString filePath() { return path; }
    inline FileInfo fileInfo() { return info; }

    inline int numFiles() { return nfiles; }
    inline int numErrors() { return nerrors; }
    inline qint64 numBytes() { return gotbytes; }
    inline qint64 totalBytes() { return totbytes; }

private:
    void scanFile(const QFileInfo &qfi);
    void scanDir(const QFileInfo &qfi);
    void report();
    void reportDone();

private slots:
    void fileProgress();
    void dirProgress();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Scan::Options)


/**
 * Private helper class representing a work item for the scanner thread
 */
class ScanItem : public QObject
{
    friend class Scan;
    friend class Scanner;
    friend class ScanCoder;
    Q_OBJECT

private:
    const QString path;
    OpaqueInfo datainfo;
    QString errmsg;
    qint64 gotbytes;
    bool done;
    bool cancel;

    inline ScanItem(Scan *scan);

signals:
    void progress();
};


/**
 * Private helper class representing the asynchronous scanner thread.
 */
class Scanner : public QThread
{
    friend class Scan;
    friend class ScanCoder;

    QMutex mutex;
    QWaitCondition cond;


    Scanner();
    virtual void run();

    ScanItem *getwork();
    void scan(Scan::Options opts);
    void scanFile(ScanItem *item);
};


/**
 * OpaqueCoder subclass that chunkifies and encodes opaque data
 * and adds the appropriate chunks to the ShareStore.
 */
class ScanCoder : public OpaqueCoder
{
    ScanItem *item;

    virtual bool dataChunk(const QByteArray &chunk,
                const OpaqueKey &key,
                qint64 ofs, qint32 size);
    virtual bool keyChunk(const QByteArray &chunk,
                const OpaqueKey &key,
                const QList<OpaqueKey> &subkeys);

public:
    inline ScanCoder(QObject *parent, ScanItem *item)
        : OpaqueCoder(parent), item(item) { }
};
