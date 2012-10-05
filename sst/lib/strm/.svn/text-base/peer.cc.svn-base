
// XXX bug in g++ 4.1.2?  This must be declared before including QHash!?
#include <QtGlobal>
namespace SST { class Endpoint; }
uint qHash(const SST::Endpoint &ep);

#include <QtDebug>

#include "host.h"
#include "stream.h"
#include "strm/base.h"
#include "strm/peer.h"
#include "strm/sflow.h"

using namespace SST;


////////// StreamPeer //////////

StreamPeer::StreamPeer(Host *h, const QByteArray &id)
:	h(h), id(id), flow(NULL), recontimer(h), stallcount(0)
{
	Q_ASSERT(!id.isEmpty());

	// If the EID is just an encapsulated IP endpoint,
	// then also use it as a destination address hint.
	Ident ident(id);
	if (ident.scheme() == ident.IP) {
		Endpoint ep(ident.ipAddress(), ident.ipPort());
		if (ep.port == 0)
			ep.port = NETSTERIA_DEFAULT_PORT;
		addrs.insert(ep);
	}
}

StreamPeer::~StreamPeer()
{
	// Clear the state of all streams associated with this peer.
	foreach (BaseStream *bs, allstreams)
		bs->clear();
	Q_ASSERT(allstreams.isEmpty());
	Q_ASSERT(usids.isEmpty());
}

void StreamPeer::connectFlow()
{
	Q_ASSERT(!id.isEmpty());

	if (flow && flow->linkStatus() == LinkUp)
		return;	// Already have a flow working; don't need another yet.

	// XXX need a working way to determine if streams need to send...
	//if (receivers(SIGNAL(flowConnected())) == 0)
	//	return;	// No one is actually waiting for a flow.

	//qDebug() << "Lookup target" << id.toBase64();

	// Send a lookup request to each known registration server.
	foreach (RegClient *rc, h->regClients()) {
		if (!rc->registered())
			continue;	// Can't poll an inactive regserver
		if (lookups.contains(rc))
			continue;	// Already polling this regserver

		// Make sure we're hooked up to this client's signals
		conncli(rc);

		// Start the lookup, with hole punching
		lookups.insert(rc);
		rc->lookup(id, true);
	}

	// Initiate key exchange attempts to any already-known endpoints
	// using each of the network sockets we have available.
	foreach (Socket *sock, h->activeSockets()) {
		//qDebug() << this << "connectFlow: using socket" << sock;
		foreach (const Endpoint &ep, addrs)
			initiate(sock, ep);
	}

	// Keep firing off connection attempts periodically
	recontimer.start((qint64)connectRetry * 1000000);
}

void StreamPeer::conncli(RegClient *rc)
{
	if (connrcs.contains(rc))
		return;
	connrcs.insert(rc);

	// Listen for the lookup response
	connect(rc, SIGNAL(lookupDone(const QByteArray &,
			const Endpoint &, const RegInfo &)),
		this, SLOT(lookupDone(const QByteArray &,
			const Endpoint &, const RegInfo &)));

	// Also make sure we hear if this regclient disappears
	connect(rc, SIGNAL(destroyed(QObject*)),
		this, SLOT(regClientDestroyed(QObject*)));
}

void StreamPeer::lookupDone(const QByteArray &id, const Endpoint &loc,
				const RegInfo &info)
{
	if (id != this->id) {
		//qDebug() << this << "got lookupDone for wrong id"
		//	<< id.toBase64()
		//	<< "expecting" << this->id.toBase64();
		return;	// ignore responses for other lookup requests
	}

	// Mark this outstanding lookup as completed.
	RegClient *rc = (RegClient*)sender();
	if (!lookups.contains(rc)) {
		//qDebug() << "StreamPeer: unexpected lookupDone signal";
		return;	// ignore duplicates caused by concurrent requests
	}
	lookups.remove(rc);

	// If the lookup failed, notify waiting streams as appropriate.
	if (loc.isNull()) {
		qDebug() << this << "Lookup on" << id.toBase64() << "failed";
		if (!lookups.isEmpty() || !initors.isEmpty())
			return;		// There's still hope
		return flowFailed();
	}

	qDebug() << "StreamResponder::lookupDone: primary" << loc.toString()
		<< "secondaries" << info.endpoints().size();

	// Add the endpoint information we've received to our address list,
	// and initiate flow setup attempts to those endpoints.
	foundEndpoint(loc);
	foreach (const Endpoint &ep, info.endpoints())
		foundEndpoint(ep);
}

void StreamPeer::regClientDestroyed(QObject *obj)
{
	qDebug() << "StreamPeer: RegClient destroyed before lookupDone";

	RegClient *rc = (RegClient*)obj;
	lookups.remove(rc);
	connrcs.remove(rc);

	// If there are no more RegClients available at all,
	// notify waiting streams of connection failure
	// next time we get back to the main loop.
	if (lookups.isEmpty() && initors.isEmpty())
		recontimer.start(0);
}

void StreamPeer::foundEndpoint(const Endpoint &ep)
{
	Q_ASSERT(!id.isEmpty());
	Q_ASSERT(!ep.isNull());

	if (addrs.contains(ep))
		return;	// We know; sit down...

	qDebug() << "Found endpoint" << ep.toString()
		<< "for target" << id.toBase64();

	// Add this endpoint to our set
	addrs.insert(ep);

	// Attempt a connection to this endpoint
	foreach (Socket *sock, h->activeSockets())
		initiate(sock, ep);
}

void StreamPeer::initiate(Socket *sock, const Endpoint &ep)
{
	Q_ASSERT(!ep.isNull());

	// No need to initiate new flows if we already have a working one...
	if (flow && flow->linkStatus() == LinkUp)
		return;

	// Don't simultaneously initiate multiple flows to the same endpoint.
	SocketEndpoint sep(ep, sock);
	if (initors.contains(sep)) {
		//qDebug() << this << "already attmpting connection to"
		//	<< ep.toString();
		return;
	}

	qDebug() << this << "initiate to" << sep.toString();

	// Make sure our StreamResponder exists
	// to receive and dispatch incoming key exchange control packets.
	(void)h->streamResponder();

	// Create and bind a new flow
	Flow *fl = new StreamFlow(h, this, id);
	if (!fl->bind(sock, ep)) {
		qDebug() << "StreamProtocol: could not bind new flow to target"
			<< ep.toString();
		delete fl;
		return flowFailed();
	}

	// Start the key exchange process for the flow.
	// The KeyInitiator will re-parent the new flow under itself
	// for the duration of the key exchange.
	KeyInitiator *ini = new KeyInitiator(fl, magic, id);
	connect(ini, SIGNAL(completed(bool)), this, SLOT(completed(bool)));
	initors.insert(sep, ini);
	Q_ASSERT(fl->parent() == ini);
}

void StreamPeer::completed(bool success)
{
	KeyInitiator *ki = (KeyInitiator*)sender();
	Q_ASSERT(ki && ki->isDone());

	// If key agreement succeeded,
	// remove the resulting flow from the KeyInitiator
	// so it'll survive the deletion of the KeyInitiator.
	// XX we should have a way to garbage-collect flows
	// that stay inactive for a while.
	if (success)
		ki->flow()->setParent(this);

	// Remove and schedule the key initiator for deletion,
	// in case it wasn't removed already by setPrimary()
	// (e.g., if key agreement failed).
	// If the new flow hasn't been reparented by setPrimary(),
	// then it will be deleted automatically as well
	// because it is still a child of the KeyInitiator.
	SocketEndpoint sep = ki->remoteEndpoint();
	Q_ASSERT(!initors.contains(sep) || initors.value(sep) == ki);
	initors.remove(sep);
	ki->cancel();
	ki->deleteLater();
	ki = NULL;

	// If unsuccessful, notify waiting streams.
	if (!success) {
		qDebug() << "Connection attempt for ID" << id.toBase64()
			<< "to" << sep.toString() << "failed";
		if (lookups.isEmpty() && initors.isEmpty())
			return flowFailed();
		return;	// There's still hope
	}

	// We should have an active primary flow at this point,
	// since StreamFlow::start() attaches the flow if there isn't one.
	// Note: the reason we don't just set the primary right here
	// is because StreamFlow::start gets called on incoming streams too,
	// so servers don't have to initiate back-flows to their clients...
	Q_ASSERT(flow && flow->linkStatus() == LinkUp);
}

void StreamPeer::flowStarted(StreamFlow *fl)
{
	Q_ASSERT(fl->isActive());
	Q_ASSERT(fl->target() == this);
	Q_ASSERT(fl->linkStatus() == LinkUp);

	if (flow) {
		// If we already have a working primary flow,
		// we don't need a new one.
		if (flow->linkStatus() == LinkUp)
			return;	

		// But if the current primary is on the blink,
		// replace it.
		clearPrimary();
	}

	qDebug() << this << "new primary" << fl;

	// Use this flow as our primary flow for this target.
	flow = fl;
	stallcount = 0;

	// Re-parent it directly underneath us,
	// so it won't be deleted when its KeyInitiator disappears.
	fl->setParent(this);

	// Watch the link status of our primary flow,
	// so we can try to replace it if it fails.
	connect(fl, SIGNAL(linkStatusChanged(LinkStatus)),
		this, SLOT(primaryStatusChanged(LinkStatus)));

	// Notify all waiting streams
	flowConnected();
	linkStatusChanged(LinkUp);
}

void StreamPeer::clearPrimary()
{
	if (!flow)
		return;

	// Clear the primary flow
	StreamFlow *old = flow;
	flow = NULL;

	// Avoid getting further primary link status notifications from it
	disconnect(old, SIGNAL(linkStatusChanged(LinkStatus)),
		this, SLOT(primaryStatusChanged(LinkStatus)));

	// Clear all transmit-attachments
	// and return outstanding packets to the streams they came from.
	old->detachAll();
}

void StreamPeer::primaryStatusChanged(LinkStatus newstatus)
{
	qDebug() << this << "primaryStatusChanged" << newstatus;
	Q_ASSERT(flow);

	if (newstatus == LinkUp) {
		// Now that we (again?) have a working primary flow,
		// cancel and delete all outstanding KeyInitiators
		// that are still in an early enough stage
		// not to have possibly created receiver state.
		// (If we were to kill a non-early KeyInitiator,
		// the receiver might pick one of those streams
		// as _its_ primary and be left with a dangling flow!)
		stallcount = 0;
		foreach (KeyInitiator *ki, initors.values()) {
			if (!ki->isEarly())
				continue;	// too late - let it finish
			qDebug() << "deleting" << ki << "for" << id.toBase64()
				<< "to" << ki->remoteEndpoint().toString();
			Q_ASSERT(initors.value(ki->remoteEndpoint()) == ki);
			initors.remove(ki->remoteEndpoint());
			ki->cancel();
			ki->deleteLater();
		}
		return;
	}

	if (newstatus == LinkStalled) {
		if (++stallcount < stallMax) {
			qDebug() << this << "primary stall" << stallcount
				<< "of" << stallMax;
			return;
		}
	}

	// Primary is at least stalled, perhaps permanently failed -
	// start looking for alternate paths right away for quick response.
	connectFlow();

	// Pass the signal on to all streams connected to this peer.
	linkStatusChanged(newstatus);
}

void StreamPeer::retryTimeout()
{
	// If we actually have an active flow now, do nothing.
	if (flow && flow->linkStatus() == LinkUp)
		return;

	// Notify any waiting streams of failure
	if (lookups.isEmpty() && initors.isEmpty())
		flowFailed();

	// If there are (still) any waiting streams,
	// fire off a new batch of connection attempts.
	connectFlow();
}

