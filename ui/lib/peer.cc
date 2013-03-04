#include <QSettings>
#include <QtDebug>

#include "peer.h"
#include "env.h"

using namespace SST;

//=================================================================================================
// PeerTable
//=================================================================================================

uint qHash(const QPair<int,int> &p)
{
    return qHash(p.first) + qHash(p.second);
}

PeerTable::PeerTable(int numColumns, QObject *parent)
:   QAbstractTableModel(parent),
    cols(numColumns)
{
    Q_ASSERT(numColumns >= 2);

    // Set up default header labels
    hdrdata.insert(QPair<int,int>(0, Qt::DisplayRole), tr("Name"));
    hdrdata.insert(QPair<int,int>(1, Qt::DisplayRole), tr("Host ID"));
}

QList<PeerId> PeerTable::ids() const
{
    QList<PeerId> ids;
    for (int i = 0; i < peers.size(); i++)
        ids.append(peers[i].id.getId());
    return ids;
}

int PeerTable::idRow(const PeerId &hostId) const
{
    for (int i = 0; i < peers.size(); i++)
        if (peers[i].id == hostId)
            return i;
    return -1;
}

void PeerTable::useSettings(QSettings *settings, const QString &prefix)
{
    Q_ASSERT(settings);
    Q_ASSERT(!this->settings);
    Q_ASSERT(peers.isEmpty());

    this->settings = settings;
    this->settingsprefix = prefix;

    // Retrieve the user's persistent peers list
    int npeers = settings->beginReadArray(prefix);
    for (int i = 0; i < npeers; i++) {
        settings->setArrayIndex(i);
        QString name = settings->value("name").toString();
        QByteArray id = settings->value("key").toByteArray();
        if (id.isEmpty() || containsId(id)) {
            qDebug() << "Empty or duplicate peer ID";
            continue;
        }
        insertAt(i, id, name);
    }
    settings->endArray();
}

void PeerTable::insertAt(int row, const PeerId &id, const QString &name)
{
    Peer p;
    p.name = name;
    p.id = id;
    p.flags.resize(cols);
    p.flags[0] = Qt::ItemIsEnabled | Qt::ItemIsSelectable
            | Qt::ItemIsEditable;
    p.flags[1] = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    peers.insert(row, p);
}

int PeerTable::insert(const PeerId &id, QString name)
{
    int row = idRow(id);
    if (row >= 0)
        return row;

    if (name.isEmpty())
        name = tr("(unnamed peer)");

    row = peers.size();

    // Insert the new peer
    beginInsertRows(QModelIndex(), row, row);
    insertAt(row, id, name);
    endInsertRows();

    // Update our persistent peers list - @todo berkus: umm, friend list management.
    writePeers();

    // Signal anyone interested
    peerInsert(id);

    return row;
}

void PeerTable::remove(const PeerId &id)
{
    int row = idRow(id);
    if (row < 0) {
        qDebug() << "PeerTable::remove nonexistent id" << id.toString();
        return;
    }

    // Remove the peer
    beginRemoveRows(QModelIndex(), row, row);
    peers.removeAt(row);
    endRemoveRows();

    // Update our persistent peers list
    writePeers();

    // Signal anyone interested
    peerRemove(id);
}

void PeerTable::writePeers()
{
    if (!settings)
        return;

    int npeers = peers.size();
    settings->beginWriteArray(settingsprefix, npeers);
    for (int i = 0; i < npeers; i++) {
        settings->setArrayIndex(i);
        settings->setValue("name", peers[i].name);
        settings->setValue("key", peers[i].id.getId());
    }
    settings->endArray();
    settings->sync();
}

QString PeerTable::name(const PeerId &id, const QString &dflName) const
{
    int row = idRow(id);
    if (row < 0)
        return dflName;
    return name(row);
}

int PeerTable::columnCount(const QModelIndex &) const
{
    return cols;
}

int PeerTable::rowCount(const QModelIndex &) const
{
    return peers.size();
}

QVariant PeerTable::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= count())
        return QVariant();

    int col = index.column();
    if (col == 0 && (role == Qt::DisplayRole || role == Qt::EditRole))
        return peers[row].name;
    if (col == 1 && role == Qt::DisplayRole)
        return peers[row].id.toString();
    return peers[row].dyndata.value(QPair<int,int>(col,role));
}

bool PeerTable::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();
    if (row < 0 || row >= count()) {
        qWarning("PeerTable::setData: bad row index %d", row);
        return false;
    }

    int col = index.column();
    if (col < 0 || col == 1 || col >= cols) {
        qWarning("PeerTable::setData: bad column index %d", col);
        return false;
    }

    if (col == 0 && (role == Qt::DisplayRole || role == Qt::EditRole)) {

        // Changing a peer's human-readable name.
        QString str = value.toString();
        if (str.isEmpty()) {
            qWarning("PeerTable: renaming peer to empty string");
            return false;
        }

        peers[row].name = str;
        writePeers();
        dataChanged(index, index);
        return true;
    }

    // Changing some dynamic table content.
    qDebug() << "PeerTable::setData col" << col << "role" << role << "value" << value.toString();
    peers[row].dyndata.insert(QPair<int,int>(col,role), value);
    dataChanged(index, index);
    return true;
}

QVariant PeerTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    return hdrdata.value(QPair<int,int>(section,role));
}

bool PeerTable::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (orientation != Qt::Horizontal)
        return false;

    if (section < 0 || section >= cols) {
        qWarning("PeerTable::setHeaderData: bad column index %d",
            section);
        return false;
    }

    hdrdata.insert(QPair<int,int>(section,role), value);
    return true;
}

Qt::ItemFlags PeerTable::flags(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 || row >= count())
        return 0;

    int col = index.column();
    if (col < 0 || col >= cols)
        return 0;

    return peers[row].flags[col];
}

bool PeerTable::setFlags(const QModelIndex &index, Qt::ItemFlags flags)
{
    int row = index.row();
    if (row < 0 || row >= count()) {
        qWarning("PeerTable::setFlags: bad row index %d", row);
        return false;
    }

    int col = index.column();
    if (col < 0 || col >= cols) {
        qWarning("PeerTable::setFlags: bad column index %d", col);
        return false;
    }

    peers[row].flags[col] = flags;
    return true;
}

//=================================================================================================
// PeerService
//=================================================================================================

PeerService::PeerService(const QString &svname, const QString &svdesc,
                         const QString &prname, const QString &prdesc,
                         QObject *parent)
    : QObject(parent)
    , server(ssthost)
    , svname(svname)
    , prname(prname)
    , exclusive(true)
    , statcol(-1)
{
    // Handle incoming connections to our StreamServer.
    connect(&server, SIGNAL(newConnection()),
        this, SLOT(inConnection()));
    if (!server.listen(svname, svdesc, prname, prdesc))
        qCritical("PeerService: can't register service '%s' protocol '%s'",
            svname.toLocal8Bit().data(),
            prname.toLocal8Bit().data());

    recontimer.setSingleShot(true);
    connect(&recontimer, SIGNAL(timeout()), this, SLOT(reconTimeout()));
}

void PeerService::setPeerTable(PeerTable *peers)
{
    Q_ASSERT(peers);
    Q_ASSERT(!this->peers);

    this->peers = peers;

    // Watch for future changes in the peer table
    connect(peers, SIGNAL(peerInsert(const SST::PeerId&)),
        this, SLOT(peerInsert(const SST::PeerId&)));
    connect(peers, SIGNAL(peerRemove(const SST::PeerId&)),
        this, SLOT(peerRemove(const SST::PeerId&)));

    // Initiate connections to each of the currently listed peers
    foreach (const PeerId &id, peers->ids())
    {
        reconnectToPeer(id);
        updateStatus(id);
    }
}

void PeerService::setStatusColumn(int column,
                const QVariant &onlineValue,
                const QVariant &offlineValue)
{
    Q_ASSERT(peers);

    statcol = column;
    onlineval = onlineValue;
    offlineval = offlineValue;

    updateStatusAll();
}

void PeerService::updateStatusAll()
{
    if (!peers)
        return;

    foreach (const PeerId &id, peers->ids())
        updateStatus(id);
}

void PeerService::updateStatus(const PeerId &hostid)
{
    emit statusChanged(hostid);

    if (!peers || statcol < 0)
        return;

    // Find the appropriate row to update
    int row = peers->idRow(hostid);
    if (row < 0)
        return;

    qDebug() << "PeerService" << svname << prname << "update status" << peerNameOrId(hostid) << "row" << row;

    // Update the status indicator
    Stream *stream = out.value(hostid.getId());
    bool online = stream && stream->isLinkUp(); // @todo isConnected() returns true erroneously?
    QModelIndex idx = peers->index(row, statcol);
    const QVariant &val = online
            ? (onlineval.isNull() ? tr("Online") : onlineval)
            : (offlineval.isNull() ? tr("Offline") : offlineval);
    Qt::ItemDataRole role;
    if (val.type() == QVariant::Icon)
        role = Qt::DecorationRole;
    else
        role = Qt::DisplayRole;
    peers->setData(idx, val, role);
    peers->setFlags(idx, Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void PeerService::peerInsert(const PeerId &hostId)
{
    if (!peers || !peers->containsId(hostId))
        return;

    updateStatus(hostId);
    reconnectToPeer(hostId);
}

void PeerService::peerRemove(const PeerId &hostId)
{
    disconnectFromPeer(hostId);
}

QString PeerService::peerName(const PeerId &hostId, const QString &dflName) const
{
    if (peers)
        return peers->name(hostId, dflName);
    else
        return dflName;
}

Stream *PeerService::connectToPeer(const PeerId &hostId)
{
    Stream *&stream = out[hostId];
    if (stream != NULL)
        return stream;

    qDebug() << "PeerService" << svname << prname << "connecting to peer" << peerNameOrId(hostId);

    // Create a primary outgoing connection to this peer
    stream = new Stream(ssthost, this);
    stream->connectTo(hostId, svname, prname);

    // Watch for connection and disconnection events on this stream
    connect(stream, SIGNAL(linkUp()), this, SLOT(outConnected()));
    connect(stream, SIGNAL(linkDown()), this, SLOT(outDisconnected()));

    // Start the reconnection delay timer from this point.
    recontimer.start(reconDelay);

    return stream;
}

Stream *PeerService::reconnectToPeer(const PeerId &id)
{
    qDebug() << "PeerService" << svname << prname << "Attempt to reconnect peer" << id.toString();
    disconnectFromPeer(id);
    return connectToPeer(id);
}

void PeerService::disconnectFromPeer(const PeerId &id)
{
    Stream *stream = out.value(id);
    if (stream == NULL)
        return;

    out.remove(id);
    stream->deleteLater();
}

void PeerService::disconnectPeer(const PeerId &id)
{
    // First destroy any outgoing connection we may have
    disconnectFromPeer(id);

    // Then destroy incoming connections too
    foreach (Stream *stream, in.value(id))
        stream->deleteLater();
    in.remove(id);
}

void PeerService::reconTimeout()
{
    qDebug() << "PeerService" << svname << prname << "reconnect timeout";

    if (!peers)
        return;     // No peer table to track

    // Try to re-connect to each outgoing stream that isn't connected
    foreach (const PeerId &id, peers->ids())
    {
        if (!outConnected(id))
            reconnectToPeer(id);
    }
}

void PeerService::outConnected()
{
    Stream *stream = (Stream*)sender();
    Q_ASSERT(stream);

    qDebug() << "PeerService" << svname << prname << "connected";

    if (!stream->isLinkUp())
        return;     // got disconnected in the meantime

    // Update our peer table if appropriate
    updateStatus(stream->remoteHostId());

    outStreamConnected(stream);
}

void PeerService::outDisconnected()
{
    Stream *stream = (Stream*)sender();
    Q_ASSERT(stream);
    Q_ASSERT(!stream->isLinkUp());
    PeerId hostid = stream->remoteHostId();

    // Update our peer table if appropriate
    updateStatus(hostid);

    outStreamDisconnected(stream);

    // If the recontimer is already expired,
    // try to re-connect again immediately.
    // (Otherwise, wait until it expires to avoid rapid poll danger.)
    if (!recontimer.isActive())
        reconnectToPeer(hostid);
}

void PeerService::inConnection()
{
    // Process all queued incoming streams
    while (true) {
        Stream *stream = server.accept();
        if (!stream)
            break;  // No more queued incoming streams

        Q_ASSERT(stream);
        Q_ASSERT(stream->isConnected());

        PeerId id = stream->remoteHostId();
        if (!allowConnection(id)) {
            qDebug() << "PeerService" << svname << prname << "Rejected connection from" << peerNameOrId(id);
            stream->deleteLater();  // XX right way to reject?
            continue;
        }

        // Keep track of the new stream
        connect(stream, SIGNAL(linkDown()),
            this, SLOT(inDisconnected()));
        in[id].insert(stream);

        qDebug() << "PeerService" << svname << prname << "new incoming stream," << in[id].size() << "total";

        // Use this event to prod our outgoing connection attempt
        // if appropriate
        Stream *outstream = outStream(id);
        if (outstream && !outstream->isLinkUp())
            reconnectToPeer(id);

        // Forward the notification to our client
        inStreamConnected(stream);
    }
}

bool PeerService::allowConnection(const PeerId &hostId)
{
    if (!exclusive)
        return true;
    if (!peers)
        return false;
    return peers->containsId(hostId);
}

void PeerService::inDisconnected()
{
    qDebug() << "PeerService" << svname << prname << "Incoming stream disconnected";

    Stream *stream = (Stream*)sender();
    Q_ASSERT(stream);
    Q_ASSERT(!stream->isConnected());

    PeerId hostid = stream->remoteHostId();

    QSet<Stream*> &inset = in[hostid];
    bool contained = inset.remove(stream);
    if (inset.isEmpty())
        in.remove(hostid);

    // Notify our client
    if (contained)
        inStreamDisconnected(stream);

    // Update our peer table if appropriate
    updateStatus(hostid);

    stream->deleteLater();
}

