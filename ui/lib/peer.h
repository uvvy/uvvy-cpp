class PeerService : public QObject
{
    QTimer recontimer;                      // To reconnect failed streams
    bool exclusive;

    // This stuff is not really needed for peerservice, only for peertable updater
    QPointer<PeerTable> peers;              // Peer table to track

    // For online status reporting
    int statcol;                // PeerTable status column
    QVariant onlineval;         // Value to set when online
    QVariant offlineval;        // Value to set when offline


public:
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
