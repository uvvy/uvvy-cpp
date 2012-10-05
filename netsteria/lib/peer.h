#ifndef PEER_H
#define PEER_H

#include <QPair>
#include <QList>
#include <QHash>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QByteArray>
#include <QAbstractTableModel>

#include "stream.h"

class QSettings;


// Qt table model describing a list of peers.
// The first column always lists the human-readable peer names,
// and the second column always lists the base64 encodings of the host IDs.
// (A UI would typically hide the second column except in "advanced" mode.)
// An arbitrary number of additional columns can hold dynamic content:
// e.g., online status information about each peer.
class PeerTable : public QAbstractTableModel
{
	Q_OBJECT

private:
	const int cols;

	struct Peer {
		// Basic peer info
		QString name;
		QByteArray id;

		// Data defining the dynamic content, by <column, role>
		QHash<QPair<int,int>,QVariant> dyndata;

		// Item flags for each item
		QVector<Qt::ItemFlags> flags;
	};
	QList<Peer> peers;

	// Data defining the table headers, by <column, role>.
	QHash<QPair<int,int>,QVariant> hdrdata;

	// Settings object we use to make the peer table persistent.
	QPointer<QSettings> settings;
	QString settingsprefix;

public:
	PeerTable(int numColumns = 2, QObject *parent = NULL);

	// Set up the PeerTable to keep its state persistent
	// in the specified QSettings object under the given prefix.
	// This method immediately reads the peer names from the settings,
	// then holds onto the settings and writes any updates to it.
	// Must be called when PeerTable is freshly created and still empty.
	void useSettings(QSettings *settings, const QString &prefix);

	inline int count() const { return peers.size(); }

	// Return the name and host ID of a peer by row number
	inline QString name(int row) const
		{ return peers[row].name; }
	inline QByteArray id(int row) const
		{ return peers[row].id; }

	// Return a list of all peers' host IDs.
	QList<QByteArray> ids() const;

	// Return the row number of a peer by its host ID, -1 if no such peer.
	int idRow(const QByteArray &hostId) const;
	inline bool containsId(const QByteArray &hostId) const
		{ return idRow(hostId) >= 0; }

	// Return the name of a peer by host ID, defaultName if no such peer.
	QString name(const QByteArray &hostId,
			const QString &defaultName = tr("unknown peer")) const;

	// Insert or remove a peer
	int insert(const QByteArray &id, QString name);
	void remove(const QByteArray &id);

	// QAbstractItemModel virtual methods
	virtual int columnCount(const QModelIndex &parent = QModelIndex())
				const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex() )
				const;
	virtual QVariant data(const QModelIndex &index,
				int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value,
				int role = Qt::EditRole);
	virtual QVariant headerData(int section, Qt::Orientation orientation,
				int role = Qt::DisplayRole) const;
	virtual bool setHeaderData(int section, Qt::Orientation orientation,
				const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setFlags(const QModelIndex &index, Qt::ItemFlags flags);

signals:
	// These signals provide slightly simpler alternatives to
	// beginInsertRow, endInsertRow, beginInsertColumn, endInsertColumn.
	void peerInsert(const QByteArray &id);
	void peerRemove(const QByteArray &id);

private:
	void insertAt(int row, const QByteArray &id, const QString &name);
	void writePeers();
};


// This class represents a peer-to-peer service,
// consisting of a StreamServer to accept incoming connections from peers,
// and a set of outgoing streams, one per interesting host ID.
// Can be set up to track the peers in a given PeerTable automatically
// and maintain service availability/status information in the PeerTable.
class PeerService : public QObject
{
	Q_OBJECT

public:
	typedef SST::Stream Stream;
	typedef SST::StreamServer StreamServer;

private:
	// Minimum delay between successive connection attempts - 1 minute
	static const int reconDelay = 60*1000;

	StreamServer server;			// To accept incoming streams
	const QString svname;			// Name of service we provide
	const QString prname;			// Name of protocol we support
	QHash<QByteArray, Stream*> out;		// Outoing streams by host ID
	QHash<QByteArray, QSet<Stream*> > in;	// Incoming streams by host ID
	QPointer<PeerTable> peers;		// Peer table to track
	QTimer recontimer;			// To reconnect failed streams
	bool exclusive;

	// For online status reporting
	int statcol;				// PeerTable status column
	QVariant onlineval;			// Value to set when online
	QVariant offlineval;			// Value to set when offline


public:
	PeerService(const QString &serviceName, const QString &serviceDesc,
			const QString &protoName, const QString &protoDesc,
			QObject *parent = NULL);

	// Set this PeerService to track the peers in a given PeerTable,
	// automatically maintaining an outgoing stream to each listed pear.
	void setPeerTable(PeerTable *peers);
	inline PeerTable *peerTable() { return peers; }

	// Set us up to keep a particular column of our PeerTable updated
	// with reports of the on-line status of this service for each peer.
	// Caller can optionally specify the particular display values to use
	// for "online" and "offline" status.
	void setStatusColumn(int column,
				const QVariant &onlineValue = QVariant(),
				const QVariant &offlineValue = QVariant());
	inline void clearStatusColumn() { setStatusColumn(-1); }

	// Create a primary outgoing connection if one doesn't already exist.
	Stream *connectToPeer(const QByteArray &hostId);

	// Create a new outgoing connection to a given peer,
	// destroying the old primary connection if any.
	Stream *reconnectToPeer(const QByteArray &hostId);

	// Destroy any outgoing connection we may have to a given peer.
	void disconnectFromPeer(const QByteArray &hostId);

	// Destroy all connections, outgoing AND incoming, with a given peer.
	void disconnectPeer(const QByteArray &hostId);

	// Return the current outgoing stream to a given peer, NULL if none.
	inline Stream *outStream(const QByteArray &hostId)
		{ return out.value(hostId); }

	// Returns true if an outgoing stream exists and is connected.
	inline bool outConnected(const QByteArray &id)
		{ Stream *s = out.value(id); return s && s->isConnected(); }

	inline QSet<Stream*> inStreams(const QByteArray &hostId)
		{ return in.value(hostId); }

	// Return the name of a given peer, or 'defaultname' if unknown
	virtual QString peerName(const QByteArray &hostId,
			const QString &defaultName = tr("unknown peer")) const;

	// Return the name of a given peer, or its Base64 host ID if unknown.
	inline QString peerNameOrId(const QByteArray &hostId)
		{ return peerName(hostId, hostId.toBase64()); }

protected:
	// Decide whether to allow an incoming stream from a particular host.
	// The default implementation returns true if 'exclusive' is clear;
	// otherwise it only returns true if the host is listed in 'peers'.
	virtual bool allowConnection(const QByteArray &hostId);

	// Update the status indicators in our PeerTable for a given peer.
	virtual void updateStatus(const QByteArray &id);

	// Update the status indicators for all peers in our PeerTable.
	void updateStatusAll();

signals:
	void outStreamConnected(Stream *strm);
	void outStreamDisconnected(Stream *strm);
	void inStreamConnected(Stream *strm);
	void inStreamDisconnected(Stream *strm);
	void statusChanged(const QByteArray &id);

private:
	void deletePrimary();

private slots:
	void peerInsert(const QByteArray &id);
	void peerRemove(const QByteArray &id);
	void outConnected();
	void outDisconnected();
	void inConnection();
	void inDisconnected();
	void reconTimeout();
};


#endif	// PEER_H
