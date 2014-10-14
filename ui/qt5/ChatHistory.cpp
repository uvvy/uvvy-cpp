#include "chathistory.h"
#include "mk4.h"
#include <QDir>

//probably add database encryption directly to Metakit if not yet supported?
class ChatHistory::Private
{
public:
    std::unique_ptr<c4_Storage> storage;
    std::unique_ptr<c4_View> view;

    Private(sss::peer_identity const& id)
    {
        appdir.mkdir("ChatHistory");
        // Use base32 for naming the files, it's more filesystem-compatible.
        // this here uses proquint currently.
        QString filename = QString::fromUtf8(id.to_string().c_str());
        QString name = appdir.path() + "/ChatHistory/history." + filename;

        storage = make_unique<c4_Storage>(name.toLocal8Bit().constData(), /*read-write-flag:*/ 1);
        view = make_unique<c4_View>(
            storage->GetAs(
                "chathistory["
                    "unread:B,"
                    "originator:S,"
                    "originator_eid:S,"
                    "timestamp:T,"
                    "target:S,"
                    "msg_hash:S,"
                    "msg:S]"));
    }
};

ChatHistory::ChatHistory(sss::peer_identity const& id, QObject* parent)
    : QObject(parent)
    , m_pimpl(std::make_shared<Private>(id))
{}

ChatHistory::~ChatHistory()
{
    m_pimpl->storage->Commit();
}

void ChatHistory::insertHistoryLine(const QString& originator, const QDateTime timestamp, const QString& message)
{
    c4_Row row;
    c4_StringProp pOriginator("originator");
    c4_StringProp pMsg("msg");

    pOriginator(row) = originator.toUtf8().constData();
    pMsg(row) = message.toUtf8().constData();

    m_pimpl->view->Add(row);
}

void ChatHistory::newHistorySynced()
{
}
