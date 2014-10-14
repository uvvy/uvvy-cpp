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
namespace sss {
    class host;
    class peer_identity;
}

/**
 * Qt item model describing a list of chat messages.
 * There are roles bindings for QML:
 *
 * - avatarUrl
 *
 * An arbitrary number of additional columns can hold dynamic content:
 * e.g., online status information about each peer.
 */
class ChatModel : public QAbstractItemModel
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;

public:
    ChatModel(std::shared_ptr<sss::host> h, QObject *parent = 0);

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

// private:
    // Internal use only.
    void updateData(int row);
private:
    void insertAt(int row, sss::peer_identity const& eid, QString const& name);

signals:
    // These signals provide slightly simpler alternatives to
    // beginInsertRow, endInsertRow, beginInsertColumn, endInsertColumn.
    void peerInserted(sss::peer_identity const& eid);
    void peerRemoved(sss::peer_identity const& eid);
};
