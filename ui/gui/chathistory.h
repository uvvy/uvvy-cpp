#pragma once

//
// @todo: Should be a Store?
//
#include <QObject>
#include <QByteArray>
#include <QDateTime>
// #include "store.h"
#include "mk4.h"
#include "peerid.h"

// For now, store a file per chat ID.
class ChatHistory : public QObject
{
	Q_OBJECT

	c4_Storage* storage;
	c4_View* view;
	// Store* historyStore;

public:
	/**
	 * @param id binary chat identifier.
	 */
	ChatHistory(const SST::PeerId& id, QObject* parent = 0);
	~ChatHistory();

public slots:
	void insertHistoryLine(const QString& originator, const QDateTime timestamp, const QString& message);

private slots:
	void newHistorySynced();
};
