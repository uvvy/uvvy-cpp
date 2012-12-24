#pragma once

#include <QByteArray>
#include <QDebug>
#include "base32.h"

namespace SST
{

/**
 * Peer ID - helper for keeping all peer-related ID conversions in one place.
 * Contains a binary peer identifier plus methods to convert it into a string representation.
 */
class PeerId
{
    QByteArray id;

public:
    PeerId() : id() {} 
    PeerId(QString base32);
    PeerId(QByteArray id);

    QByteArray getId() const { return id; }
    QString toString() const;
    bool isEmpty() const { return id.isEmpty(); }
};

inline QDebug& operator << (QDebug& ts, const PeerId& id)
{
    return ts << id.toString();
}

inline uint qHash(const PeerId& key) { return qHash(key.getId()); }
inline bool operator == (const PeerId& a, const PeerId& b) { return a.getId() == b.getId(); }
inline bool operator != (const PeerId& a, const PeerId& b) { return a.getId() != b.getId(); }

//=====================================================================================================================
// PeerId
//=====================================================================================================================

inline PeerId::PeerId(QString base32)
    : id(Encode::fromBase32(base32))
{
}

inline PeerId::PeerId(QByteArray id_)
    : id(id_)
{
}

inline QString PeerId::toString() const
{
    return Encode::toBase32(id);
}

} // namespace SST
