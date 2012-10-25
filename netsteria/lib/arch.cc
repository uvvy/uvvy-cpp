#include <string.h>

#include <QDir>

#include "arch.h"
#include "env.h"
#include "xdr.h"
#include "sha2.h"

using namespace SST;


////////// Archive //////////

Archive *Archive::primary;

void Archive::init(const QDir &appdir)
{
    primary = new Archive(appdir.path() + "/archive");
}


Archive::Archive(const QString &filename)
:   file(filename),
    readpos(0),
    readidx(0),
    verify(true)
{
    // Open unbuffered in hopes of ensuring atomicity of appends,
    // in case more than one process has the log open at once.
    // (Probably a bad idea, but be as robust as possible.)
    file.open(QFile::ReadWrite | QFile::Append |
            QFile::Unbuffered);
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner |
                QFile::ReadUser | QFile::WriteUser);

    if (file.size() == 0) {

        // Creating a new archive - initialize the header.
        QByteArray ch;
        XdrStream ws(&ch, QIODevice::WriteOnly);
        ws << (quint32)VXA_ARCHSIG;
        writeChunk(ch);

    } else {

        // Read existing log header
        QByteArray ch = readChunk(0);

        XdrStream rs(ch);
        quint32 sig;
        rs >> sig;
        if (rs.status() != rs.Ok || sig != VXA_ARCHSIG) {
            Q_ASSERT(0);    // XXX
        }
    }
}

bool Archive::readHeader(quint64 pos, quint64 &check, qint32 &size)
{
    // Read the chunk header at the appropriate file offset
    if (!file.seek(pos))
        return false;
    QByteArray hdr = file.read(hdrsize);
    if (hdr.size() < hdrsize)
        return false;           // End of log file

    // Decode the header
    XdrStream rs(&hdr, QIODevice::ReadOnly);
    qint32 sig;
    rs >> sig >> size >> check;

    // Check the chunk signature
    if (rs.status() != rs.Ok || sig != VXA_CHUNKSIG || size <= 0)
        return false;           // Bad chunk

    return true;
}

QByteArray Archive::readAt(quint64 pos, quint64 expcheck, int expsize)
{
    // Read the chunk header at the appropriate file offset
    quint64 check;
    qint32 size;
    if (!readHeader(pos, check, size))
        return QByteArray();

    // Check chunk parameters against expected ones
    if (expsize != 0 && size != expsize) {
        qDebug("Archive::readChunk: size mismatch");
        return QByteArray();
    }
    if (expcheck != 0 && check != expcheck) {
        qDebug("Archive::readChunk: check mismatch");
        return QByteArray();        // Check mismatch
    }

    // Now read the chunk data
    return file.read(size);
}

QByteArray Archive::readChunk(ArchiveIndex idx, quint64 expcheck, int expsize)
{
    Q_ASSERT(idx < chunks.size());
    quint64 pos = chunks.at(idx);
    return readAt(pos, expcheck, expsize);
}

QByteArray Archive::readChunk(const QByteArray &ohash, int expsize)
{
    OuterCheck check = hashCheck(ohash);

    // Find all the archived chunks having this check value.
    QHash<OuterCheck, ArchiveIndex>::const_iterator i = chash.find(check);
    while (i != chash.end() && i.key() == check) {

        // We'll only know if the chunk really matches by reading it.
        QByteArray ch = readChunk(i.value(), check, expsize);
        if (!ch.isEmpty() && Sha512::hash(ch) == ohash)
            return ch;

        qDebug("Archive::readChunk: check collision");
        ++i;
    }

    // We apparently don't have this chunk.
    return QByteArray();
}

QByteArray Archive::readStore(const QByteArray &ohash)
{
    return readChunk(ohash);    // XXX always full hash provided?
}

QByteArray Archive::writeChunk(const QByteArray &data)
{
    Q_ASSERT(!data.isEmpty());

    QByteArray ohash = Sha512::hash(data);
    OuterCheck check = hashCheck(ohash);

    // Don't write the same chunk twice
    scan();
    if (!readChunk(ohash, data.size()).isEmpty())
        return ohash;

    // Prepend the chunk header to the chunk data
    QByteArray buf;
    XdrStream ws(&buf, QIODevice::WriteOnly);
    ws << (quint32)VXA_CHUNKSIG << (quint32)data.size() << check;
    ws.writeRawData(data.data(), data.size());

    // Write it all to the log in one (hopefully atomic) append.
    if (file.write(buf) < buf.size()) {
        qWarning("Error writing to archive (disk full?)");
            // XXX
        return QByteArray();
    }

    // Get the newly-written chunk into our indexes.
    scan();

    // Optionally verify that the chunk really was written correctly.
    if (verify && readChunk(ohash, data.size()).isEmpty()) {
        qWarning("Verify failed after writing to archive"); //XXX
        return QByteArray();
    }

    return ohash;
}

bool Archive::scan()
{
    while (readpos < file.size()) {
        Q_ASSERT(readidx == chunks.size());

        quint64 check;
        qint32 size;
        if (!readHeader(readpos, check, size)) {
            qFatal("Corrupt archive");  // XXX
        }
        qDebug("Found chunk - size %d check %llx", size, check);

        // Calculate and append the chunk info
        chunks.append(readpos);

        // Insert the chunk into the ocheck-keyed hash
        chash.insert(check, readidx);

        readpos += hdrsize + size;
        readidx++;
    }
    return true;
}

bool Archive::test()
{
    qFatal("not implemented");
    return false;
}
