#ifndef MAIN_H
#define MAIN_H

#include <QSet>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QUdpSocket>
#include <QBasicTimer>

#include "reg.h"
#include "util.h"

namespace SST {

class XdrStream;
class RegServer;


// We maintain a RegRecord record for each registered client.
// For memory space efficiency, we keep info blocks in binary form
// and only break them out into a RegInfo object when we need to.
// Private helper class for RegServer.
class RegRecord : public QObject
{
	Q_OBJECT
	friend class RegServer;

	RegServer * const srv;
	const QByteArray id;
	const QByteArray nhi;
	const Endpoint ep;
	const QByteArray info;
	QBasicTimer timer;


	RegRecord(RegServer *srv, const QByteArray &id, const QByteArray &nhi,
		const Endpoint &ep, const QByteArray &info);
	~RegRecord();

	// Re-implement QObject's timerEvent() to handle a client timeout.
	void timerEvent(QTimerEvent *evt);

	// Break the description into keywords,
	// and either insert or remove the keyword entries for this record.
	void regKeywords(bool insert);
};


class RegServer : public QObject
{
	Q_OBJECT
	friend class RegRecord;

	QUdpSocket sock;

	// XX should timeout periodically
	QByteArray secret;

	// Hash of insert challenge cookies and corresponding responses
	QHash<QByteArray,QByteArray> chalhash;

	// Hash table to look up records by ID
	QHash<QByteArray,RegRecord*> idhash;

	// Hash table to look up records by case-insensitive keyword
	QHash<QString,QSet<RegRecord*> > kwhash;

	// Set of all existing records, for empty searches
	QSet<RegRecord*> allrecords;


public:
	RegServer();

private:
	void udpDispatch(QByteArray &msg, const Endpoint &ep);
	void doInsert1(XdrStream &rxs, const Endpoint &ep);
	void doInsert2(XdrStream &rxs, const Endpoint &ep);
	void doLookup(XdrStream &rxs, const Endpoint &ep);
	void doSearch(XdrStream &rxs, const Endpoint &ep);

	void replyInsert1(const Endpoint &ep, const QByteArray &idi,
				const QByteArray &nhi);
	void replyLookup(RegRecord *reci, quint32 replycode,
			const QByteArray &idr, RegRecord *recr);
	QByteArray calcCookie(const Endpoint &ep, const QByteArray &idi,
				const QByteArray &nhi);
	RegRecord *findCaller(const Endpoint &ep, const QByteArray &idi,
				const QByteArray &nhi);

private slots:
	void udpReadyRead();
};

} // namespace SST

#endif	// MAIN_H
