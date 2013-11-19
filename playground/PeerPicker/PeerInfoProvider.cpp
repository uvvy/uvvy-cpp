// encapsulate  and connect it to regservers from settings_provider
// query all peers or query filtered peer list
// enter them into a table model
#include "PeerInfoProvider.h"
#include "private/regserver_client.h"
#include "client_profile.h"
#include "client_utils.h"
#include "settings_provider.h"
#include "logging.h"

using namespace std;
using namespace ssu;
using namespace uia::routing;

class PeerInfoProvider::Private
{
public:
    internal::regserver_client client;
    QList<pair<peer_id, client_profile>> peers;

    Private(ssu::host *h)
        : client(h)
    {
        client.on_ready.connect([this] {
            logger::debug() << "client ready";
            client.search(""); // Trigger listing of all available peers.
        });
        client.on_disconnected.connect([] {
            logger::debug() << "client disconnected";
        });
        client.on_lookup_done.connect([this](ssu::peer_id const& peer,
            ssu::endpoint const&,
            uia::routing::client_profile const& profile)
        {
            update_peer_profile(peer, profile);
        });
        client.on_lookup_failed.connect([](ssu::peer_id const& peer) {
            logger::debug() << "lookup failed";
        });

        client.on_search_done.connect([this](std::string const&,
            std::vector<ssu::peer_id> const& peers,
            bool last)
        {
            found_peers(peers, last);
        });
    }

    void update_peer_profile(ssu::peer_id const& peer,
            uia::routing::client_profile const& profile)
    {
        logger::debug() << "update_peer_profile " << peer;
    }

    void found_peers(std::vector<ssu::peer_id> const& peers, bool last)
    {
        for (auto peer : peers) {
            logger::debug() << "found_peers " << peer;
            // peers[peer] = ??;
        }
    }

    void connect_regservers()
    {
        auto settings = settings_provider::instance();
        regclient_set_profile(settings.get(), client, client.get_host());
        regclient_connect_regservers(settings.get(), client);
    }
};

PeerInfoProvider::PeerInfoProvider(shared_ptr<ssu::host> h, QObject *parent)
    : QAbstractTableModel(parent)
    , m_pimpl(make_shared<Private>(h.get()))
{
    m_pimpl->connect_regservers();
}

int PeerInfoProvider::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_pimpl->peers.size();
}

int PeerInfoProvider::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant PeerInfoProvider::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= ssize_t(m_pimpl->peers.size()) || index.row() < 0) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        auto pair = m_pimpl->peers.at(index.row());

        if (index.column() == 0) {
            return pair.first.to_string().c_str();
        }
        else if (index.column() == 1) {
            return pair.second.host_name().c_str();
        }
        else if (index.column() == 2) {
            return pair.second.owner_firstname().c_str();
        }
        else if (index.column() == 3) {
            return pair.second.owner_nickname().c_str();
        }
        else if (index.column() == 4) {
            return pair.second.owner_lastname().c_str();
        }
        else if (index.column() == 5) {
            return pair.second.city().c_str();
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
            case 0: return tr("EID");
            case 1: return tr("Host name");
            case 2: return tr("First name");
            case 3: return tr("Nickname");
            case 4: return tr("Last name");
            case 5: return tr("Address");
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

    for (int row=0; row < rows; row++)
    {
        pair<peer_id, client_profile> pair;
        m_pimpl->peers.insert(position, pair);
    }

    endInsertRows();
    return true;
}

bool PeerInfoProvider::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        m_pimpl->peers.removeAt(position);
    }

    endRemoveRows();
    return true;
}

bool PeerInfoProvider::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // if (index.isValid() && role == Qt::EditRole) {
        // int row = index.row();

        // QPair<QString, QString> p = m_pimpl->peers.value(row);

        // if (index.column() == 0) {
        //     p.first = value.toString();
        // }
        // else if (index.column() == 1) {
        //     p.second = value.toString();
        // }
        // else {
            // return false;
        // }

        // m_pimpl->peers.replace(row, p);
        // emit(dataChanged(index, index));

        // return true;
    // }

    return false;
}

Qt::ItemFlags PeerInfoProvider::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractTableModel::flags(index);// | Qt::ItemIsEditable;
}
