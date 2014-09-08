//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <QAbstractItemModel>
// #include <memory>

class settings_provider;
namespace ssu {
    class host;
    class peer_identity;
}

/**
 * Qt item model describing a list of peers.
 * There are roles bindings for QML:
 *
 * - firstName
 * - lastName
 * - nickName
 * - mood
 * - eid
 * - email
 * - avatarUrl
 *
 * An arbitrary number of additional columns can hold dynamic content:
 * e.g., online status information about each peer.
 */
class ContactModel : public QAbstractItemModel
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    ContactModel(std::shared_ptr<ssu::host> h, QObject *parent = 0);
    ContactModel(std::shared_ptr<ssu::host> h, std::shared_ptr<settings_provider> s, QObject *parent = 0);

    // QAbstractItemModel methods for QML:
    QHash<int, QByteArray> roleNames() const override;

    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    // AbstractTableModel methods;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
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
    int insert(ssu::peer_identity const& eid, QString name);
    /**
     * Remove a peer.
     */
    void remove(ssu::peer_identity const& eid);
    /**
     * Return the name of a peer by row number.
     */
    QString name(int row) const;
    /**
     * Return the name of a peer by host ID, defaultName if no such peer.
     */
    QString name(ssu::peer_identity const& eid, QString const& defaultName = tr("(unnamed peer)")) const;
    /**
     * Return the host ID of a peer by row number.
     */
    ssu::peer_identity id(int row) const;
    /**
     * Return the row number of a peer by its host ID, -1 if no such peer.
     */
    int rowWithId(ssu::peer_identity const& eid) const;

    inline bool containsId(ssu::peer_identity const& eid) const {
        return rowWithId(eid) >= 0;
    }

    /**
     * Return a list of all peers' host IDs.
     */
    QList<ssu::peer_identity> ids() const;

// private:
    // Internal use only.
    void updateData(int row);
private:
    /**
     * Set up the PeerTableModel to keep its state persistent
     * in the specified settings_provider object under name "peers".
     * This method immediately reads the peer names from the settings,
     * then holds onto the settings and writes any updates to it.
     * Must be called when PeerTableModel is freshly created and still empty
     * (that's why there's a separate constructor you should use).
     */
    void useSettings(std::shared_ptr<settings_provider> settings);

    void insertAt(int row, ssu::peer_identity const& eid, QString const& name);
    void writeSettings();

signals:
    // These signals provide slightly simpler alternatives to
    // beginInsertRow, endInsertRow, beginInsertColumn, endInsertColumn.
    void peerInserted(ssu::peer_identity const& eid);
    void peerRemoved(ssu::peer_identity const& eid);
};
