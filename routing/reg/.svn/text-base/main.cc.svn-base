
#include <QByteArray>
#include <QStringList>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QtDebug>

#include "main.h"
#include "ident.h"
#include "util.h"
#include "reg.h"
#include "xdr.h"
#include "sha2.h"

using namespace SST;


#define TIMEOUT_SEC	(1*60*60)	// Records last 1 hour

#define MAX_RESULTS	100		// Maximum number of search results


// RegServer implementation

RegServer::RegServer()
{
	int port = REGSERVER_DEFAULT_PORT;
	if (!sock.bind(port, QUdpSocket::DontShareAddress))
		qFatal("Can't bind to UDP port %d: %s",
			port, sock.errorString().toLocal8Bit().data());
	Q_ASSERT(sock.state() == sock.BoundState);
	qDebug() << "RegServer bound on port" << port;

	connect(&sock, SIGNAL(readyRead()), this, SLOT(udpReadyRead()));
}

void
RegServer::udpReadyRead()
{
	QByteArray msg;
	int size;
	while ((size = sock.pendingDatagramSize()) >= 0) {
		msg.resize(size);
		Endpoint ep;
		if (sock.readDatagram(msg.data(), size,
				&ep.addr, &ep.port) != size)
			qWarning("Error receiving %d-byte UDP datagram", size);
		udpDispatch(msg, ep);
	}
}

void
RegServer::udpDispatch(QByteArray &msg, const Endpoint &srcep)
{
	//qDebug("Received %d-byte message from %s:%d",
	//	msg.size(), srcep.addr.toString().toAscii().data(), srcep.port);

	XdrStream rxs(msg);
	quint32 magic, code;
	rxs >> magic >> code;
	if (rxs.status() != rxs.Ok || magic != REG_MAGIC) {
		qDebug("Received message from %s:%d with bad magic",
			srcep.addr.toString().toAscii().data(), srcep.port);
		return;
	}

	switch (code) {
	case REG_REQUEST | REG_INSERT1:
		return doInsert1(rxs, srcep);
	case REG_REQUEST | REG_INSERT2:
		return doInsert2(rxs, srcep);
	case REG_REQUEST | REG_LOOKUP:
		return doLookup(rxs, srcep);
	case REG_REQUEST | REG_SEARCH:
		return doSearch(rxs, srcep);
	default:
		qDebug("Received message from %s:%d with bad request code",
			srcep.addr.toString().toAscii().data(), srcep.port);
	}
}

void
RegServer::doInsert1(XdrStream &rxs, const Endpoint &srcep)
{
	//qDebug("Insert1");

	// Decode the rest of the request message (after the 32-bit code)
	QByteArray idi, nhi;
	rxs >> idi >> nhi;
	if (rxs.status() != rxs.Ok || idi.isEmpty()) {
		qDebug("Received invalid Insert1 message");
		return;
	}

	// Compute and reply with an appropriate challenge.
	replyInsert1(srcep, idi, nhi);
}

void
RegServer::replyInsert1(const Endpoint &srcep, const QByteArray &idi,
			const QByteArray &nhi)
{
	// Compute the correct challenge cookie for the message.
	// XX really should use a proper HMAC here.
	QByteArray chal = calcCookie(srcep, idi, nhi);

	// Send back the challenge cookie in our INSERT1 response,
	// in order to verify round-trip connectivity
	// before spending CPU time checking the client's signature.
	QByteArray resp;
	XdrStream wxs(&resp, QIODevice::WriteOnly);
	wxs << REG_MAGIC << (quint32)(REG_RESPONSE | REG_INSERT1)
		<< nhi << chal;
	sock.writeDatagram(resp, srcep.addr, srcep.port);
}

QByteArray RegServer::calcCookie(const Endpoint &srcep, const QByteArray &idi,
				const QByteArray &nhi)
{
	// Make sure we have a host secret to key the challenge with
	if (secret.isEmpty())
		secret = randBytes(SHA256_DIGEST_LENGTH);
	Q_ASSERT(secret.size() == SHA256_DIGEST_LENGTH);

	// Compute the correct challenge cookie for the message.
	// XX really should use a proper HMAC here.
	Sha256 chalsha;
	XdrStream chalwxs(&chalsha);
	chalwxs << secret << srcep.addr.toString() << srcep.port
		<< idi << nhi << secret;
	return chalsha.final();
}

void
RegServer::doInsert2(XdrStream &rxs, const Endpoint &srcep)
{
	//qDebug("Insert2");

	// Decode the rest of the request message (after the 32-bit code)
	QByteArray idi, ni, chal, info, key, sig;
	rxs >> idi >> ni >> chal >> info >> key >> sig;
	if (rxs.status() != rxs.Ok || idi.isEmpty()) {
		qDebug("Received invalid Insert2 message");
		return;
	}

	// The client's INSERT1 contains the hash of its nonce;
	// the INSERT2 contains the actual nonce,
	// so that an eavesdropper can't easily forge an INSERT2
	// after seeing the client's INSERT1 fly past.
	QByteArray nhi = Sha256::hash(ni);

	// First check the challenge cookie:
	// if it is invalid (perhaps just because our secret expired),
	// just send back a new INSERT1 response.
	if (calcCookie(srcep, idi, nhi) != chal) {
		qDebug("Received Insert2 message with bad cookie");
		return replyInsert1(srcep, idi, nhi);
	}

	// See if we've already responded to a request with this cookie.
	if (chalhash.contains(chal)) {
		qDebug("Received apparent replay of old Insert2 request");

		// Just return the previous response.
		// If the registered response is empty,
		// it means the client was bad so we're ignoring it:
		// in that case just silently drop the request.
		QByteArray resp = chalhash[chal];
		if (!resp.isEmpty())
			sock.writeDatagram(resp, srcep.addr, srcep.port);

		return;
	}

	// For now we only support RSA-based identities,
	// because DSA signature verification is much more costly.
	// XX would probably be good to send back an error response.
	Ident identi(idi);
	if (identi.scheme() != identi.RSA160) {
		qDebug("Received Insert for unsupported ID scheme %d",
			identi.scheme());
		chalhash.insert(chal, QByteArray());
		return;
	}

	// Parse the client's public key and make sure it matches its EID.
	if (!identi.setKey(key)) {
		qDebug("Received bad identity from client %s:%d on insert",
			srcep.addr.toString().toAscii().data(), srcep.port);
		chalhash.insert(chal, QByteArray());
		return;
	}

	// Compute the hash of the message components the client signed.
	Sha256 sigsha;
	XdrStream sigwxs(&sigsha);
	sigwxs << idi << ni << chal << info;

	// Verify the client's signature using his public key.
	if (!identi.verify(sigsha.final(), sig)) {
		qDebug("Signature check for client %s:%d failed on Insert2",
			srcep.addr.toString().toAscii().data(), srcep.port);
		chalhash.insert(chal, QByteArray());
		return;
	}

	// Insert an appropriate record into our in-memory client database.
	// This automatically replaces any existing record for the same ID,
	// in effect resetting the timeout for the client as well.
	(void)new RegRecord(this, idi, nhi, srcep, info);

	// Send a reply to the client indicating our timeout on its record,
	// so it knows how soon it will need to refresh the record.
	QByteArray resp;
	XdrStream wxs(&resp, QIODevice::WriteOnly);
	wxs << REG_MAGIC << (quint32)(REG_RESPONSE | REG_INSERT2)
		<< nhi << (quint32)TIMEOUT_SEC << srcep;
	sock.writeDatagram(resp, srcep.addr, srcep.port);

	qDebug() << "Inserted record for" << idi.toBase64() << "at"
		<< srcep.addr.toString() << srcep.port;
}

void
RegServer::doLookup(XdrStream &rxs, const Endpoint &srcep)
{
	// Decode the rest of the lookup request.
	QByteArray idi, nhi, idr;
	bool notify;
	rxs >> idi >> nhi >> idr >> notify;
	if (rxs.status() != rxs.Ok || idi.isEmpty()) {
		qDebug("Received invalid Lookup message");
		return;
	}
	if (notify)
		qDebug("Lookup with notify");

	// Lookup the initiator (caller).
	// To protect us and our clients from DoS attacks,
	// the caller must be registered with the correct source endpoint.
	RegRecord *reci = findCaller(srcep, idi, nhi);
	if (reci == NULL)
		return;

	// Return the contents of the selected record, if any, to the caller.
	// If the target is not or is no longer registered
	// (e.g., because its record timed out since
	// the caller's last Lookup or Search request that found it),
	// respond to the initiator anyway indicating as such.
	RegRecord *recr = idhash.value(idr);
	replyLookup(reci, REG_RESPONSE | REG_LOOKUP, idr, recr);

	// Send a response to the target as well, if found,
	// so that the two can perform UDP hole punching if desired.
	if (recr && notify)
		replyLookup(recr, REG_NOTIFY | REG_LOOKUP, idi, reci);
}

void RegServer::replyLookup(RegRecord *reci, quint32 replycode,
				const QByteArray &idr, RegRecord *recr)
{
	//qDebug() << "replyLookup" << replycode;

	QByteArray resp;
	XdrStream wxs(&resp, QIODevice::WriteOnly);
	bool known = (recr != NULL);
	wxs << REG_MAGIC << replycode << reci->nhi << idr << known;
	if (known)
		wxs << recr->ep << recr->info;
	sock.writeDatagram(resp, reci->ep.addr, reci->ep.port);
}

void
RegServer::doSearch(XdrStream &rxs, const Endpoint &srcep)
{
	// Decode the rest of the search request.
	QByteArray idi, nhi;
	QString search;
	rxs >> idi >> nhi >> search;
	if (rxs.status() != rxs.Ok || idi.isEmpty()) {
		qDebug("Received invalid Search message");
		return;
	}

	// Lookup the initiator (caller) ID.
	// To protect us and our clients from DoS attacks,
	// the caller must be registered with the correct source endpoint.
	RegRecord *reci = findCaller(srcep, idi, nhi);
	if (reci == NULL)
		return;

	// Break the search string into keywords.
	// We'll interpret them as an AND-set.
	QStringList kwords = 
		search.split(QRegExp("\\W+"), QString::SkipEmptyParts);

	// Find the keyword with fewest matches to start with,
	// in order to make the set arithmetic reasonable efficient.
	QSet<RegRecord*> minset;
	QString minkw;
	int mincount = INT_MAX;
	foreach (QString kw, kwords) {
		QSet<RegRecord*> set = kwhash.value(kw);
		if (set.size() < mincount) {
			minset = set;
			mincount = set.size();
			minkw = kw;
		}
	}
	//qDebug() << "Min keyword" << minkw << "set size" << mincount;

	// From there, narrow the minset further for each keyword.
	foreach (QString kw, kwords) {
		if (minset.isEmpty())
			break;	// Can't get any smaller than this...
		if (kw == minkw)
			continue; // It's the one we started with
		minset.intersect(kwhash[kw]);
	}
	//qDebug() << "Minset size" << minset.size();

	// If client supplied no keywords, (try to) return all records.
	const QSet<RegRecord*> &results
		= kwords.isEmpty() ? allrecords : minset;

	// Limit the set of results to at most MAX_RESULTS.
	qint32 nresults = results.size();
	bool complete = true;
	if (nresults > MAX_RESULTS) {
		nresults = MAX_RESULTS;
		complete = false;
	}

	// Return the IDs of the selected records to the caller.
	QByteArray resp;
	XdrStream wxs(&resp, QIODevice::WriteOnly);
	wxs << REG_MAGIC << (quint32)(REG_RESPONSE | REG_SEARCH)
		<< nhi << search << complete << nresults;
	foreach (RegRecord *rec, results) {
		qDebug() << "search result" << rec->id.toBase64();
		wxs << rec->id;
		if (--nresults == 0)
			break;
	}
	Q_ASSERT(nresults == 0);
	sock.writeDatagram(resp, srcep.addr, srcep.port);
}

RegRecord *RegServer::findCaller(const Endpoint &ep, const QByteArray &idi,
				const QByteArray &nhi)
{
	RegRecord *reci = idhash.value(idi);
	if (reci == NULL) {
		qDebug("Received request from non-registered caller");
		return NULL;
	}
	if (ep != reci->ep) {
		qDebug("Received request from wrong source endpoint");
		return NULL;
	}
	if (nhi != reci->nhi) {
		qDebug("Received request with incorrect hashed nonce");
		return NULL;
	}
	return reci;
}



// RegRecord implementation

RegRecord::RegRecord(RegServer *srv,
		const QByteArray &id, const QByteArray &nhi,
		const Endpoint &ep, const QByteArray &info)
:	srv(srv), id(id), nhi(nhi), ep(ep), info(info)
{
	// Register us in the RegServer's ID-lookup table,
	// replacing any existing entry with this ID.
	RegRecord *old = srv->idhash.value(id);
	if (old != NULL) {
		qDebug() << "Replacing existing record for"
			<< id.toBase64();
		delete old;
	}
	srv->idhash[id] = this;
	srv->allrecords += this;

	// Register all our keywords in the RegServer's keyword table.
	regKeywords(true);

	// Set the record's timeout
	timer.start(TIMEOUT_SEC * 1000, this);
}

RegRecord::~RegRecord()
{
	//qDebug("~RegRecord");

	Q_ASSERT(srv->idhash.value(id) == this);
	srv->idhash.remove(id);
	srv->allrecords.remove(this);

	regKeywords(false);
}

void RegRecord::regKeywords(bool insert)
{
	foreach (QString kw, RegInfo(info).keywords()) {
		QSet<RegRecord*> &set = srv->kwhash[kw];
		if (insert) {
			set.insert(this);
		} else {
			set.remove(this);
			if (set.isEmpty())
				srv->kwhash.remove(kw);
		}
	}
}

void RegRecord::timerEvent(QTimerEvent *)
{
	qDebug() << "Timed out record for" << id.toBase64() << "at"
		<< ep.addr.toString() << ep.port;

	// Our timeout expired - just silently delete this record.
	deleteLater();
}


// Main application entrypoint

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	RegServer regserv;
	return app.exec();
}

