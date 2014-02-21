//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QVector>
#include <QDebug>
#include "ContactModel.h"
#include "routing/private/regserver_client.h"
#include "routing/client_profile.h"
#include "client_utils.h"
#include "arsenal/settings_provider.h"
#include "arsenal/logging.h"

using namespace std;
using namespace ssu;
using namespace uia::routing;

// @todo move to public API?
enum {
    Name = Qt::UserRole,
    EID,
    Host,
    Owner_FirstName,
    Owner_LastName,
    Owner_NickName,
    City,
    AvatarURL,
    MaxRoles
};

struct Peer
{
    static const int version_ = 1;

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
        initFlags();
    }
    Peer(peer_id eid)
        : name_(), eid_(eid), profile_()
    {
        initFlags();
    }

    void initFlags()
    {
        // FIXME: probably not applicable for generic itemmodel?
        flags_.resize(1);
        for (auto& f : flags_) {
            f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }
        flags_[0] |= Qt::ItemIsEditable;
    }
};

const int Peer::version_;

flurry::oarchive& operator << (flurry::oarchive& out, QString const& str)
{
    out << std::string(str.toUtf8().constData());
    return out;
}

flurry::iarchive& operator >> (flurry::iarchive& in, QString& str)
{
    std::string s;
    in >> s;
    str = QString::fromUtf8(s.c_str());
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

//=================================================================================================
// ContactModel private implementation
//=================================================================================================

class ContactModel::Private
{
public:
    ContactModel* parent_;
    internal::regserver_client client_;
    QList<Peer> peers_;

    /**
     * Data defining the table headers, by <column, role>.
     */
    QHash<QPair<int,int>, QVariant> headers_;

    // Save/load peer list in settings if not nullptr
    shared_ptr<settings_provider> settings_;

    Private(ContactModel* parent, ssu::host *h)
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
            updatePeerProfile(peer, profile);
        });
        client_.on_lookup_failed.connect([](ssu::peer_id const& peer) {
            logger::debug() << "lookup failed";
        });

        client_.on_search_done.connect([this](std::string const&,
            std::vector<ssu::peer_id> const& peers,
            bool last)
        {
            foundPeers(peers, last);
        });
    }

    void updatePeerProfile(ssu::peer_id const& peer,
            uia::routing::client_profile const& profile)
    {
        logger::debug() << "updatePeerProfile " << peer;
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

    void foundPeers(std::vector<ssu::peer_id> const& peers, bool last)
    {
        for (auto peer : peers) {
            logger::debug() << "foundPeers " << peer;
            if (!containsId(peer)) {
                peers_.append(Peer(peer));
            }
            client_.lookup(peer);
        }
    }

    void connectRegservers()
    {
        auto settings = settings_provider::instance();
        regclient_set_profile(settings.get(), client_, client_.get_host());
        regclient_connect_regservers(settings.get(), client_);
    }
};

ContactModel::ContactModel(shared_ptr<ssu::host> h, QObject *parent)
    : QAbstractItemModel(parent)
    , m_pimpl(make_shared<Private>(this, h.get()))
{
    m_pimpl->connectRegservers();
}

ContactModel::ContactModel(shared_ptr<ssu::host> h, shared_ptr<settings_provider> s,
    QObject *parent)
    : QAbstractItemModel(parent)
    , m_pimpl(make_shared<Private>(this, h.get()))
{
    useSettings(s);
    m_pimpl->connectRegservers();
}

QModelIndex ContactModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

QModelIndex ContactModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column, &m_pimpl->peers_[row]);
}

int ContactModel::insert(ssu::peer_id const& eid, QString name)
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
    writeSettings();

    // Signal anyone interested
    Q_EMIT peerInserted(eid);

    return row;
}

void ContactModel::remove(ssu::peer_id const& eid)
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
    writeSettings();

    // Signal anyone interested
    Q_EMIT peerRemoved(eid);
}

QString ContactModel::name(int row) const
{
    return m_pimpl->peers_[row].name_;
}

QString ContactModel::name(ssu::peer_id const& eid, QString const& defaultName) const
{
    int row = rowWithId(eid);
    if (row < 0)
        return defaultName;
    return name(row);
}

ssu::peer_id ContactModel::id(int row) const
{
    return m_pimpl->peers_[row].eid_;
}

QList<ssu::peer_id> ContactModel::ids() const
{
    QList<ssu::peer_id> ids;
    for (auto& peer : m_pimpl->peers_) {
        ids.append(peer.eid_);
    }
    return ids;
}

int ContactModel::rowWithId(ssu::peer_id const& eid) const
{
    for (int i = 0; i < m_pimpl->peers_.size(); i++)
        if (m_pimpl->peers_[i].eid_ == eid)
            return i;
    return -1;
}

int ContactModel::count() const
{
    return m_pimpl->peers_.size();
}

int ContactModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return count();
}

int ContactModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
    // return MaxColumns;
}

void ContactModel::insertAt(int row, ssu::peer_id const& eid, QString const& name)
{
    Peer p;
    p.name_ = name;
    p.eid_ = eid;

    m_pimpl->peers_.insert(row, p);
}

bool ContactModel::insertRows(int position, int rows, const QModelIndex &index)
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

bool ContactModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        m_pimpl->peers_.removeAt(position);
    }

    endRemoveRows();
    return true;
}

void ContactModel::updateData(int row)
{
    // NB: this resets all selections made by user...
    /*reset();*/ // Until I fix model indexes, full reload will do.
    // emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex())));
}

QVariant ContactModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= ssize_t(m_pimpl->peers_.size()) || index.row() < 0) {
        return QVariant();
    }
    Q_ASSERT(index.column() == 0);

    auto peer = m_pimpl->peers_.at(index.row());

    if (role == Name) {
        return peer.name_;
    }
    else if (role == EID) {
        return peer.eid_.to_string().c_str();
    }
    else if (role == Host) {
        return peer.profile_.host_name().c_str();
    }
    else if (role == Owner_FirstName) {
        return peer.profile_.owner_firstname().c_str();
    }
    else if (role == Owner_LastName) {
        return peer.profile_.owner_lastname().c_str();
    }
    else if (role == Owner_NickName) {
        return peer.profile_.owner_nickname().c_str();
    }
    else if (role == City) {
        return peer.profile_.city().c_str();
    }
    return QVariant();
    // return peer.additional_data_.value(QPair<int,int>(column, role));
}

bool ContactModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();
    if (row < 0 or row >= count()) {
        return false;
    }
    Q_ASSERT(index.column() == 0);

    if (role == Name)
    {
        // Changing a peer's human-readable name.
        QString str = value.toString();
        if (str.isEmpty()) {
            str = tr("(unnamed peer)");
        }

        m_pimpl->peers_[row].name_ = str;
        writeSettings();
        dataChanged(index, index);
        return true;
    }
    else if (role == EID) {
        return false;
    }
    else if (role == Host) {
        return false;
    }
    else if (role == Owner_FirstName) {
        return false;
    }
    else if (role == Owner_LastName) {
        return false;
    }
    else if (role == Owner_NickName) {
        return false;
    }
    else if (role == City) {
        return false;
    }

    // Changing some dynamic table content.
    // m_pimpl->peers_[row].additional_data_.insert(QPair<int,int>(column,role), value);
    // Q_EMIT dataChanged(index, index);
    // return true;
    return false;
}

Qt::ItemFlags ContactModel::flags(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 or row >= count()) {
        return 0;
    }

    if (index.column() != 0) {
        return 0;
    }

    return m_pimpl->peers_[row].flags_[0];
}

bool ContactModel::setFlags(const QModelIndex &index, Qt::ItemFlags flags)
{
    int row = index.row();
    if (row < 0 or row >= count()) {
        return false;
    }

    if (index.column() != 0) {
        return false;
    }

    m_pimpl->peers_[row].flags_[0] = flags;
    return true;
}

void ContactModel::useSettings(std::shared_ptr<settings_provider> settings)
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

void ContactModel::writeSettings()
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

QHash<int, QByteArray> ContactModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Name] = "displayName";
    roles[EID] = "eid";
    roles[Host] = "host";
    roles[Owner_FirstName] = "firstName";
    roles[Owner_LastName] = "lastName";
    roles[Owner_NickName] = "nickName";
    roles[City] = "city";
    roles[AvatarURL] = "avatarUrl";
    // "mood"
    // "email"
    return roles;
}
