// encapsulate  and connect it to regservers from settings_provider
// query all peers or query filtered peer list
// enter them into a table model
#include "PeerInfoProvider.h"
#include "private/regserver_client.h"

using namespace std;

class PeerInfoProvider::Private
{
    uia::routing::internal::regserver_client client;
public:
    Private(ssu::host *h)
        : client(h)
    {}
    void connectRegservers() {}
};

PeerInfoProvider::PeerInfoProvider(shared_ptr<ssu::host> h, QObject *parent)
    : QAbstractTableModel(parent)
    , m_pimpl(make_shared<Private>(h.get()))
{
    m_pimpl->connectRegservers();
}

PeerInfoProvider::PeerInfoProvider(QList<QPair<QString,QString>> pairs, QObject *parent)
    : QAbstractTableModel(parent)
{
    listOfPairs = pairs;
}

int PeerInfoProvider::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return listOfPairs.size();
}

int PeerInfoProvider::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant PeerInfoProvider::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= listOfPairs.size() || index.row() < 0) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        QPair<QString, QString> pair = listOfPairs.at(index.row());

        if (index.column() == 0) {
            return pair.first;
        }
        else if (index.column() == 1) {
            return pair.second;
        }
    }
    return QVariant();
}

QVariant PeerInfoProvider::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return tr("Name");

            case 1:
                return tr("Address");

            default:
                return QVariant();
        }
    }
    return QVariant();
}

bool PeerInfoProvider::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; row++) {
        QPair<QString, QString> pair(" ", " ");
        listOfPairs.insert(position, pair);
    }

    endInsertRows();
    return true;
}

bool PeerInfoProvider::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        listOfPairs.removeAt(position);
    }

    endRemoveRows();
    return true;
}

bool PeerInfoProvider::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        int row = index.row();

        QPair<QString, QString> p = listOfPairs.value(row);

        if (index.column() == 0) {
            p.first = value.toString();
        }
        else if (index.column() == 1) {
            p.second = value.toString();
        }
        else {
            return false;
        }

        listOfPairs.replace(row, p);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

Qt::ItemFlags PeerInfoProvider::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
