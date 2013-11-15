#pragma once

#include <QAbstractTableModel>
#include <QPair>
#include <QList>

namespace ssu {
    class host;
}

class PeerInfoProvider : public QAbstractTableModel
{
    Q_OBJECT
    class Private;
    std::shared_ptr<Private> m_pimpl;
    QList<QPair<QString,QString>> listOfPairs;

public:
    PeerInfoProvider(std::shared_ptr<ssu::host> h, QObject *parent = 0);
    PeerInfoProvider(QList<QPair<QString,QString>> listofPairs, QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());
    QList<QPair<QString,QString>> getList();
};
