#pragma once

#include <QAbstractTableModel>
#include <QPair>
#include <QList>

namespace ssu {
    class host;
}

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
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole) override;
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex()) override;

    // Internal use only.
    void updateData(int row);
};
