#pragma once

#include <QPair>
#include <QList>
#include <QHash>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QByteArray>
#include <QAbstractTableModel>
#include "stream.h"
#include "peerid.h"

class QSettings;

/** @class QAbstractTableModel
 * Qt's abstract Model/View class.
 */

/**
 * Qt table model describing a list of peers.
 * The first column always lists the human-readable peer names,
 * and the second column always lists the base64 encodings of the host IDs.
 * (A UI would typically hide the second column except in "advanced" mode.)
 * An arbitrary number of additional columns can hold dynamic content:
 * e.g., online status information about each peer.
 */
class PeerTable : public QAbstractTableModel
{
    Q_OBJECT

private:
    const int cols;

    struct Peer {
        // Basic peer info
        QString name;
        SST::PeerId id;

        /**
         * Data defining the dynamic content, by <column, role>
         */
        QHash<QPair<int,int>,QVariant> dyndata;

        /**
         * Item flags for each item
         */
        QVector<Qt::ItemFlags> flags;
    };
    QList<Peer> peers;

    /**
     * Data defining the table headers, by <column, role>.
     */
    QHash<QPair<int,int>,QVariant> hdrdata;

    /**
     * Settings object we use to make the peer table persistent.
     */
    QPointer<QSettings> settings;
    QString settingsprefix;

public:
    PeerTable(int numColumns = 2, QObject *parent = NULL);

    /**
     * Set up the PeerTable to keep its state persistent
     * in the specified QSettings object under the given prefix.
     * This method immediately reads the peer names from the settings,
     * then holds onto the settings and writes any updates to it.
     * Must be called when PeerTable is freshly created and still empty.
     * @param settings [description]
     * @param prefix   [description]
     */
    void useSettings(QSettings *settings, const QString &prefix);

    inline int count() const { return peers.size(); }

    /**
     * Return the name of a peer by row number.
     * @param  row [description]
     * @return     [description]
     */
    inline QString name(int row) const { return peers[row].name; }

    /**
     * Return the host ID of a peer by row number.
     * @param  row [description]
     * @return     [description]
     */
    inline SST::PeerId id(int row) const { return peers[row].id; }

    /**
     * Return a list of all peers' host IDs.
     */
    QList<SST::PeerId> ids() const;

    /**
     * Return the row number of a peer by its host ID, -1 if no such peer.
     * @param  hostId [description]
     * @return        [description]
     */
    int idRow(const SST::PeerId &hostId) const;
    inline bool containsId(const SST::PeerId &hostId) const
        { return idRow(hostId) >= 0; }

    /**
     * Return the name of a peer by host ID, defaultName if no such peer.
     * @param  hostId      [description]
     * @param  defaultName [description]
     * @return             [description]
     */
    QString name(const SST::PeerId &hostId,
            const QString &defaultName = tr("unknown peer")) const;

    /**
     * Insert a peer.
     * @param  id   [description]
     * @param  name [description]
     * @return      [description]
     */
    int insert(const SST::PeerId &id, QString name);
    /**
     * Remove a peer.
     * @param id [description]
     */
    void remove(const SST::PeerId &id);

    // QAbstractItemModel virtual methods
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex() ) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setFlags(const QModelIndex &index, Qt::ItemFlags flags);

signals:
    // These signals provide slightly simpler alternatives to
    // beginInsertRow, endInsertRow, beginInsertColumn, endInsertColumn.
    void peerInsert(const SST::PeerId &id);
    void peerRemove(const SST::PeerId &id);

private:
    void insertAt(int row, const SST::PeerId &id, const QString &name);
    void writePeers();
};

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
    QHash<SST::PeerId, Stream*> out;         // Outoing streams by host ID
    QHash<SST::PeerId, QSet<Stream*> > in;   // Incoming streams by host ID
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
     * @return        [description]
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
    inline Stream *outStream(const SST::PeerId &hostId)
        { return out.value(hostId.getId()); }

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
     * Return the name of a given peer, or 'defaultname' if unknown
     * @param  hostId      [description]
     * @param  defaultName [description]
     * @return             [description]
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
