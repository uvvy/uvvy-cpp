//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QVector>
#include <QDebug>
#include "PeerTableModel.h"
#include "private/regserver_client.h"
#include "client_profile.h"
#include "client_utils.h"
#include "settings_provider.h"
#include "logging.h"

using namespace std;
using namespace ssu;
using namespace uia::routing;

// @todo move to public API?
enum {
    Name = 0,
    EID = 1,
    Host = 2,
    Owner_FirstName = 3,
    Owner_LastName = 4,
    Owner_NickName = 5,
    City = 6,
    MaxColumns
};

struct Peer
{
    const int version_ = 1;

    // Basic peer info - always present
    QString name_;
    peer_id eid_;
    client_profile profile_;
    QHash<QPair<int,int>, QVariant> additional_data_;
    /**
     * Item flags for each item.
     */
    QVector<Qt::ItemFlags> flags_;

    Peer()
    {
        init_flags();
    }
    Peer(peer_id eid)
        : name_(), eid_(eid), profile_()
    {
        init_flags();
    }

    void init_flags()
    {
        flags_.resize(MaxColumns);
        for (auto& f : flags_) {
            f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }
        flags_[0] = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
};

flurry::oarchive& operator << (flurry::oarchive& out, QString const& str)
{
    out << std::string(str.toUtf8().constData());
    return out;
}

flurry::iarchive& operator >> (flurry::iarchive& in, QString& str)
{
    std::string s;
    in >> s;
    str = QString(s.c_str()); // @fixme That is a fromAscii() conversion...
    return in;
}

flurry::oarchive& operator << (flurry::oarchive& out, Peer const& peer)
{
    out << peer.version_ << peer.name_ << peer.eid_ << peer.profile_;
    return out;
}

flurry::iarchive& operator >> (flurry::iarchive& in, Peer& peer)
{
    int version;
    in >> version >> peer.name_ >> peer.eid_ >> peer.profile_;
    if (version != 1)
        throw runtime_error("unsupported Peer structure serialization version");
    return in;
}

class PeerTableModel::Private
{
public:
    PeerTableModel* parent_;
    internal::regserver_client client_;
    QList<Peer> peers_;

    /**
     * Data defining the table headers, by <column, role>.
     */
    QHash<QPair<int,int>, QVariant> headers_;

    // Save/load peer list in settings if not nullptr
    shared_ptr<settings_provider> settings_;

    Private(PeerTableModel* parent, ssu::host *h)
        : parent_(parent)
        , client_(h)
    {
        // Set up default header labels
        headers_.insert(QPair<int,int>(Name, Qt::DisplayRole), tr("Name"));
        headers_.insert(QPair<int,int>(EID, Qt::DisplayRole), tr("Host ID"));
        headers_.insert(QPair<int,int>(Host, Qt::DisplayRole), tr("Host name"));
        headers_.insert(QPair<int,int>(Owner_FirstName, Qt::DisplayRole), tr("First name"));
        headers_.insert(QPair<int,int>(Owner_LastName, Qt::DisplayRole), tr("Last name"));
        headers_.insert(QPair<int,int>(Owner_NickName, Qt::DisplayRole), tr("Nickname"));
        headers_.insert(QPair<int,int>(City, Qt::DisplayRole), tr("City"));

        client_.on_ready.connect([this] {
            logger::debug() << "client ready";
            client_.search(""); // Trigger listing of all available peers.
        });
        client_.on_disconnected.connect([] {
            logger::debug() << "client disconnected";
        });
        client_.on_lookup_done.connect([this](ssu::peer_id const& peer,
            ssu::endpoint const&,
            uia::routing::client_profile const& profile)
        {
            update_peer_profile(peer, profile);
        });
        client_.on_lookup_failed.connect([](ssu::peer_id const& peer) {
            logger::debug() << "lookup failed";
        });

        client_.on_search_done.connect([this](std::string const&,
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
        // find in peers entry with peer id "peer" and update it's profile.
        int i = 0;
        foreach (auto& pair, peers_)
        {
            if (pair.eid_ == peer)
            {
                peers_[i].profile_ = profile;
                parent_->updateData(i);
                break;
            }
            ++i;
        }
    }

    bool containsId(ssu::peer_id const& eid) const
    {
        for (auto& peer : peers_) {
            if (peer.eid_ == eid) {
                return true;
            }
        }
        return false;
    }

    void found_peers(std::vector<ssu::peer_id> const& peers, bool last)
    {
        for (auto peer : peers) {
            logger::debug() << "found_peers " << peer;
            if (!containsId(peer)) {
                peers_.append(Peer(peer));
            }
            client_.lookup(peer);
        }
    }

    void connect_regservers()
    {
        auto settings = settings_provider::instance();
        regclient_set_profile(settings.get(), client_, client_.get_host());
        regclient_connect_regservers(settings.get(), client_);
    }
};

PeerTableModel::PeerTableModel(shared_ptr<ssu::host> h, QObject *parent)
    : QAbstractTableModel(parent)
    , m_pimpl(make_shared<Private>(this, h.get()))
{
    m_pimpl->connect_regservers();
}

PeerTableModel::PeerTableModel(shared_ptr<ssu::host> h, shared_ptr<settings_provider> s,
    QObject *parent)
    : QAbstractTableModel(parent)
    , m_pimpl(make_shared<Private>(this, h.get()))
{
    use_settings(s);
    m_pimpl->connect_regservers();
}

int PeerTableModel::insert(ssu::peer_id const& eid, QString name)
{
    int row = rowWithId(eid);
    if (row >= 0)
        return row;

    if (name.isEmpty())
        name = tr("(unnamed peer)");

    row = m_pimpl->peers_.size();

    // Insert the new peer
    beginInsertRows(QModelIndex(), row, row);
    insertAt(row, eid, name);
    endInsertRows();

    // Update our persistent peers list - @todo berkus: umm, friend list management.
    write_settings();

    // Signal anyone interested
    Q_EMIT peerInserted(eid);

    return row;
}

void PeerTableModel::remove(ssu::peer_id const& eid)
{
    int row = rowWithId(eid);
    if (row < 0) {
        return;
    }

    // Remove the peer
    beginRemoveRows(QModelIndex(), row, row);
    m_pimpl->peers_.removeAt(row);
    endRemoveRows();

    // Update our persistent peers list
    write_settings();

    // Signal anyone interested
    Q_EMIT peerRemoved(eid);
}

QString PeerTableModel::name(int row) const
{
    return m_pimpl->peers_[row].name_;
}

QString PeerTableModel::name(ssu::peer_id const& eid, QString const& defaultName) const
{
    int row = rowWithId(eid);
    if (row < 0)
        return defaultName;
    return name(row);
}

ssu::peer_id PeerTableModel::id(int row) const
{
    return m_pimpl->peers_[row].eid_;
}

QList<ssu::peer_id> PeerTableModel::ids() const
{
    QList<ssu::peer_id> ids;
    for (auto& peer : m_pimpl->peers_) {
        ids.append(peer.eid_);
    }
    return ids;
}

int PeerTableModel::rowWithId(ssu::peer_id const& eid) const
{
    for (int i = 0; i < m_pimpl->peers_.size(); i++)
        if (m_pimpl->peers_[i].eid_ == eid)
            return i;
    return -1;
}

int PeerTableModel::count() const
{
    return m_pimpl->peers_.size();
}

int PeerTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return count();
}

int PeerTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return MaxColumns;
}

void PeerTableModel::insertAt(int row, ssu::peer_id const& eid, QString const& name)
{
    Peer p;
    p.name_ = name;
    p.eid_ = eid;

    m_pimpl->peers_.insert(row, p);
}

bool PeerTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; row++)
    {
        Peer peer;
        m_pimpl->peers_.insert(position, peer);
    }

    endInsertRows();
    return true;
}

bool PeerTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        m_pimpl->peers_.removeAt(position);
    }

    endRemoveRows();
    return true;
}

void PeerTableModel::updateData(int row)
{
    // NB: this resets all selections made by user...
    reset(); // Until I fix model indexes, full reload will do.
    // emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex())));
}

QVariant PeerTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= ssize_t(m_pimpl->peers_.size()) || index.row() < 0) {
        return QVariant();
    }

    auto peer = m_pimpl->peers_.at(index.row());

    int column = index.column();
    if (column == Name and (role == Qt::DisplayRole or role == Qt::EditRole))
        return peer.name_;

    if (role == Qt::DisplayRole) {
        if (column == EID) {
            return peer.eid_.to_string().c_str();
        }
        else if (column == Host) {
            return peer.profile_.host_name().c_str();
        }
        else if (column == Owner_FirstName) {
            return peer.profile_.owner_firstname().c_str();
        }
        else if (column == Owner_LastName) {
            return peer.profile_.owner_lastname().c_str();
        }
        else if (column == Owner_NickName) {
            return peer.profile_.owner_nickname().c_str();
        }
        else if (column == City) {
            return peer.profile_.city().c_str();
        }
    }
    return peer.additional_data_.value(QPair<int,int>(column, role));
}

bool PeerTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();
    if (row < 0 or row >= count()) {
        return false;
    }

    int column = index.column();
    if (column < 0 or column == 1 or column >= MaxColumns) {
        return false;
    }

    if (role == Qt::DisplayRole or role == Qt::EditRole)
    {
        if (column == Name)
        {
            // Changing a peer's human-readable name.
            QString str = value.toString();
            if (str.isEmpty()) {
                str = tr("(unnamed peer)");
            }

            m_pimpl->peers_[row].name_ = str;
            write_settings();
            dataChanged(index, index);
            return true;
        }
        else if (column == EID) {
            return false;
        }
        else if (column == Host) {
            return false;
        }
        else if (column == Owner_FirstName) {
            return false;
        }
        else if (column == Owner_LastName) {
            return false;
        }
        else if (column == Owner_NickName) {
            return false;
        }
        else if (column == City) {
            return false;
        }
    }

    // Changing some dynamic table content.
    m_pimpl->peers_[row].additional_data_.insert(QPair<int,int>(column,role), value);
    Q_EMIT dataChanged(index, index);
    return true;
}

QVariant PeerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    return m_pimpl->headers_.value(QPair<int,int>(section, role));
}

bool PeerTableModel::setHeaderData(int section, Qt::Orientation orientation,
    const QVariant& value, int role)
{
    if (orientation != Qt::Horizontal) {
        return false;
    }

    if (section < 0 || section >= MaxColumns) {
        return false;
    }

    m_pimpl->headers_.insert(QPair<int,int>(section,role), value);
    return true;
}

Qt::ItemFlags PeerTableModel::flags(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 or row >= count()) {
        return 0;
    }

    int col = index.column();
    if (col < 0 or col >= MaxColumns) {
        return 0;
    }

    return m_pimpl->peers_[row].flags_[col];
}

bool PeerTableModel::setFlags(const QModelIndex &index, Qt::ItemFlags flags)
{
    int row = index.row();
    if (row < 0 or row >= count()) {
        return false;
    }

    int col = index.column();
    if (col < 0 or col >= MaxColumns) {
        return false;
    }

    m_pimpl->peers_[row].flags_[col] = flags;
    return true;
}

void PeerTableModel::use_settings(std::shared_ptr<settings_provider> settings)
{
    Q_ASSERT(!m_pimpl->settings_);
    Q_ASSERT(settings);
    Q_ASSERT(m_pimpl->peers_.isEmpty());

    m_pimpl->settings_ = settings;
    byte_array data(settings->get_byte_array("peers"));

    if (data.size() > 0)
    {
        byte_array_iwrap<flurry::iarchive> read(data);
        std::vector<Peer> in_peers;
        read.archive() >> in_peers;
        for (auto& peer : in_peers) {
            if (peer.eid_.is_empty() or containsId(peer.eid_)) {
                qDebug() << "Empty or duplicate peer ID";
                continue;
            }
            m_pimpl->peers_.append(peer);
        }
    }
}

void PeerTableModel::write_settings()
{
    if (!m_pimpl->settings_)
        return;

    byte_array data;
    {
        byte_array_owrap<flurry::oarchive> write(data);
        std::vector<Peer> out_peers;
        for (auto& peer : m_pimpl->peers_) {
            out_peers.emplace_back(peer);
        }
        write.archive() << out_peers;
    }

    m_pimpl->settings_->set("peers", data);
    m_pimpl->settings_->sync();
}
