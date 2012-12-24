#include "chathistory.h"
#include "env.h"
#include <QDir>

ChatHistory::ChatHistory(const SST::PeerId& id, QObject* parent)
	: QObject(parent)
{
	appdir.mkdir("ChatHistory");
	// Use base32 for naming the files, it's more filesystem-compatible.
	// QString filename = QString(Base32(id));
	QString filename = id.toString();
	QString name = appdir.path() + "/ChatHistory/history." + filename;
	storage = new c4_Storage(name.toLocal8Bit().constData(), /*read-write-flag:*/ 1);
	view = new c4_View(storage->GetAs("chathistory[unread:B,originator:S,originator_eid:S,timestamp:T,target:S,msg_hash:S,msg:S]"));
}

ChatHistory::~ChatHistory()
{
	storage->Commit();

	delete view;
	delete storage;
}

void ChatHistory::insertHistoryLine(const QString& originator, const QDateTime timestamp, const QString& message)
{
	c4_Row row;
	c4_StringProp pOriginator("originator");
	c4_StringProp pMsg("msg");

	pOriginator(row) = originator.toUtf8().constData();
	pMsg(row) = message.toUtf8().constData();

	view->Add(row);
}

void ChatHistory::newHistorySynced()
{
}
