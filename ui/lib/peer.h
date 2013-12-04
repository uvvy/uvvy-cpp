/**
 * This class represents a peer-to-peer service,
 * consisting of a StreamServer to accept incoming connections from peers,
 * and a set of outgoing streams, one per interesting host ID.
 * Can be set up to track the peers in a given PeerTable automatically
 * and maintain service availability/status information in the PeerTable.
 */
class PeerService : public QObject
{
    Q_OBJECT

public:
    typedef SST::Stream Stream;
    typedef SST::StreamServer StreamServer;

private:
    /**
     * Minimum delay between successive connection attempts - 1 minute
     */
    static const int reconDelay = 60*1000;

    StreamServer server;                    // To accept incoming streams
    const QString svname;                   // Name of service we provide
    const QString prname;                   // Name of protocol we support
    QHash<SST::PeerId, Stream*> out;        // Outoing streams by host ID
    QHash<SST::PeerId, QSet<Stream*> > in;  // Incoming streams by host ID
    QPointer<PeerTable> peers;              // Peer table to track
    QTimer recontimer;                      // To reconnect failed streams
    bool exclusive;

    // For online status reporting
    int statcol;                // PeerTable status column
    QVariant onlineval;         // Value to set when online
    QVariant offlineval;        // Value to set when offline


public:
    PeerService(const QString &serviceName, const QString &serviceDesc,
            const QString &protoName, const QString &protoDesc,
            QObject *parent = NULL);

    /**
     * Set this PeerService to track the peers in a given PeerTable,
     * automatically maintaining an outgoing stream to each listed pear.
     * @param peers [description]
     */
    void setPeerTable(PeerTable *peers);
    inline PeerTable *peerTable() { return peers; }

    /**
     * Set us up to keep a particular column of our PeerTable updated
     * with reports of the on-line status of this service for each peer.
     * Caller can optionally specify the particular display values to use
     * for "online" and "offline" status.
     * @param column       [description]
     * @param onlineValue  [description]
     * @param offlineValue [description]
     */
    void setStatusColumn(int column,
                const QVariant &onlineValue = QVariant(),
                const QVariant &offlineValue = QVariant());
    inline void clearStatusColumn() { setStatusColumn(-1); }

    /**
     * Create a primary outgoing connection if one doesn't already exist.
     * @param  hostId target host EID to connect to.
     * @return a new Stream connection to the given peer.
     */
    Stream *connectToPeer(const SST::PeerId &hostId);

    /**
     * Create a new outgoing connection to a given peer, destroying the old primary connection if any.
     * @param  hostId [description]
     * @return        [description]
     */
    Stream *reconnectToPeer(const SST::PeerId &hostId);

    /**
     * Destroy any outgoing connection we may have to a given peer.
     * @param hostId [description]
     */
    void disconnectFromPeer(const SST::PeerId &hostId);

    /**
     * Destroy all connections, outgoing AND incoming, with a given peer.
     * @param hostId [description]
     */
    void disconnectPeer(const SST::PeerId &hostId);

    /**
     * Return the current outgoing stream to a given peer, NULL if none.
     * @param  hostId [description]
     * @return        [description]
     */
    inline Stream *outStream(const SST::PeerId &hostId) { return out.value(hostId.getId()); }

    /**
     * Returns true if an outgoing stream exists and is connected.
     * @param  id [description]
     * @return    [description]
     */
    inline bool outConnected(const SST::PeerId &id)
        { Stream *s = out.value(id.getId()); return s && s->isConnected(); }

    /**
     * Return a set of incoming streams from given peer.
     */
    inline QSet<Stream*> inStreams(const SST::PeerId &hostId)
        { return in.value(hostId.getId()); }

    /**
     * Return the name of a given peer, or 'defaultName' if unknown
     * @param  hostId      peer identifier.
     * @param  defaultName default name to return in case no known name is found.
     * @return             peer name.
     */
    virtual QString peerName(const SST::PeerId &hostId,
            const QString &defaultName = tr("unknown peer")) const;

    /**
     * Return the name of a given peer, or its Base64 host ID if unknown.
     * @param  hostId [description]
     * @return        [description]
     */
    inline QString peerNameOrId(const SST::PeerId &hostId)
        { return peerName(hostId, hostId.toString()); }

protected:
    /**
     * Decide whether to allow an incoming stream from a particular host.
     * The default implementation returns true if 'exclusive' is not set;
     * otherwise it only returns true if the host is listed in 'peers'.
     * @param  hostId [description]
     * @return        [description]
     */
    virtual bool allowConnection(const SST::PeerId &hostId);

    /**
     * Update the status indicators in our PeerTable for a given peer.
     * @param id [description]
     */
    virtual void updateStatus(const SST::PeerId &id);

    /**
     * Update the status indicators for all peers in our PeerTable.
     */
    void updateStatusAll();

signals:
    void outStreamConnected(Stream *strm);
    void outStreamDisconnected(Stream *strm);
    void inStreamConnected(Stream *strm);
    void inStreamDisconnected(Stream *strm);
    void statusChanged(const SST::PeerId &id);

private:
    void deletePrimary();

private slots:
    void peerInsert(const SST::PeerId &id);
    void peerRemove(const SST::PeerId &id);
    void outConnected();
    void outDisconnected();
    void inConnection();
    void inDisconnected();
    void reconTimeout();
};
