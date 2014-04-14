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

void PeerService::setPeerTable(PeerTable *newPeers)
{
    Q_ASSERT(newPeers);
    Q_ASSERT(!peers);

    peers = newPeers;

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

