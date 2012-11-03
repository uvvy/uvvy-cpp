#pragma once

//
// @todo: Should be a Store?
//
#include <QObject>
#include <QByteArray>
// #include "store.h"
#include "mk4.h"

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
	ChatHistory(const QByteArray& id, QObject* parent = 0);
	~ChatHistory();

private slots:
	void newHistorySynced();
};
