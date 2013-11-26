#pragma once

#include <QAbstractTableModel>
#include <memory>

class settings_provider;
namespace ssu {
    class host;
    class peer_id;
}

/**
 * Qt table model describing a list of peers.
 * The first column always lists the human-readable peer names,
 * and the second column always lists the base64 encodings of the host IDs.
 * (A UI would typically hide the second column except in "advanced" mode.)
 * An arbitrary number of additional columns can hold dynamic content:
 * e.g., online status information about each peer.
 */
class PeerTableModel : public QAbstractTableModel
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    PeerTableModel(std::shared_ptr<ssu::host> h, QObject *parent = 0);

    // AbstractTableModel methods;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setHeaderData(int section, Qt::Orientation orientation,
        const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole) override;
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex()) override;

    int count() const;
    bool setFlags(const QModelIndex &index, Qt::ItemFlags flags);

    // SSU-relevant model functions.
    /**
     * Insert a peer.
     */
    int insert(ssu::peer_id const& eid, QString name);
    /**
     * Remove a peer.
     */
    void remove(ssu::peer_id const& eid);
    /**
     * Return the name of a peer by row number.
     */
    QString name(int row) const;
    /**
     * Return the name of a peer by host ID, defaultName if no such peer.
     */
    QString name(ssu::peer_id const& eid, QString const& defaultName = tr("(unnamed peer)")) const;
    /**
     * Return the host ID of a peer by row number.
     */
    ssu::peer_id id(int row) const;
    /**
     * Return the row number of a peer by its host ID, -1 if no such peer.
     */
    int rowWithId(ssu::peer_id const& eid) const;

    inline bool containsId(ssu::peer_id const& eid) const {
        return rowWithId(eid) >= 0;
    }

    /**
     * Return a list of all peers' host IDs.
     */
    QList<ssu::peer_id> ids() const;

    /**
     * Set up the PeerTableModel to keep its state persistent
     * in the specified settings_provider object under name "peers".
     * This method immediately reads the peer names from the settings,
     * then holds onto the settings and writes any updates to it.
     * Must be called when PeerTableModel is freshly created and still empty.
     */
    void use_settings(std::shared_ptr<settings_provider> settings);

// private:
    // Internal use only.
    void updateData(int row);
private:
    void insertAt(int row, ssu::peer_id const& eid, QString const& name);
    void write_settings();

signals:
    // These signals provide slightly simpler alternatives to
    // beginInsertRow, endInsertRow, beginInsertColumn, endInsertColumn.
    void peerInserted(ssu::peer_id const& eid);
    void peerRemoved(ssu::peer_id const& eid);
};
