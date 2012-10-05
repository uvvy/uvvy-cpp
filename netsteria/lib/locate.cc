
#include "locate.h"


////////// Locator //////////

QSet<LocationService*> Locator::services;

Locator::Locator(QObject *parent)
:	QObject(parent)
{
	locators.insert(this);
}

Locator::~Locator()
{
	locators.remove(this);
}

void Locator::insertService(LocationService *svc)
{
	Q_ASSERT(!services.contains(svc));
	services.insert(svc);
}

void Locator::removeService(LocationService *svc)
{
	// Cancel any outstanding requests to this service
	foreach (Locator *l, locators) {
		l->svconns.remove(svc);
		foreach (const QByteArray &id, l->reqs.keys())
			l->gotLocateDone(svc, id);
	}

	Q_ASSERT(services.contains(svc));
	services.remove(svc);
}

void Locator::locate(const QByteArray &id)
{
	Q_ASSERT(!id.isEmpty());

	if (services.isEmpty())
		qWarning("No location services available");

	QSet<LocationService*> &rset = reqs[id];
	foreach (LocationService *svc, services) {

		if (rset.contains(svc))
			continue; // Already requested ID from this service

		// Make sure we're hooked up to this service
		if (!svconns.contains(svc)) {
			svconns += svc;
			connect(svc, SIGNAL(found(LocationService *,
						const QByteArray &,
						const QList<Endpoint> &)),
				this, SLOT(gotFound(LocationService *,
						const QByteArray &,
						const QList<Endpoint> &)));
		}

		// Start a search for the specified ID.
		svc->locate(id);
		rset.insert(svc);
	}
}

void Locator::gotFound(LocationService *, const QByteArray &id,
		const QList<Endpoint> &addrs)
{
	// Forward the signal to our client
	found(id, addrs);
}

void Locator::gotLocateDone(LocationService *svc, const QByteArray &id)
{
	if (!reqs.contains(id)) {
		qDebug("gotLocateDone from LocationService for unknown id");
		return;
	}

	QSet<LocationService*> &rset = reqs[id];
	rset.remove(svc);
	if (!rset.isEmpty())
		return;		// Still waiting for others

	reqs.remove(id);
	locateDone(id);
}


////////// LocationService //////////

LocationService::LocationService(QObject *parent)
:	QObject(parent)
{
	Locator::insertService(this);
}

LocationService::~LocationService()
{
	Locator::removeService(this);
} 

