#include "chathistory.h"
#include "env.h"
#include <QDir>

ChatHistory::ChatHistory(const QByteArray& id, QObject* parent)
	: QObject(parent)
{
	// Use base32 for naming the files, it's more filesystem-compatible.
	// QString filename = QString(Base32(id));
	QString filename = id.toBase64();
	QString name = appdir.path() + "/ChatHistory/history." + filename;
	storage = new c4_Storage(name.toLocal8Bit().constData(), /*read-write-flag:*/ 1);
	view = new c4_View(storage->GetAs("chathistory[unread:B,originator:S,timestamp:T,target:S,msg_hash:S,msg:S]"));
}

ChatHistory::~ChatHistory()
{
	storage->Commit();

	delete view;
	delete storage;
}

void ChatHistory::newHistorySynced()
{
}
