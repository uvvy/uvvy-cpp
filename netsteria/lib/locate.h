//
// Address lookup services
//
#ifndef LOCATE_H
#define LOCATE_H

#include <QSet>
#include <QHash>
#include <QList>
#include <QPointer>

class Endpoint;
class LocationService;

class Locator : public QObject
{
	friend class LocationService;
	Q_OBJECT

	// Hash table of outstanding requests indexed by host ID
	QHash<QByteArray, QSet<LocationService*> > reqs;

	// Set of location services we're hooked up to for signaling
	QSet<LocationService*> svconns;


	// Global registry of locators and location services
	static QSet<Locator*> locators;
	static QSet<LocationService*> services;

public:
	Locator(QObject *parent);
	virtual ~Locator();

	// The subclass provides this method, to start an asynchronous search
	// for useable address information for a given host ID.
	void locate(const QByteArray &id);

signals:
	void found(const QByteArray &id, const QList<Endpoint> &addrs);
	void locateDone(const QByteArray &id);

private:
	static void insertService(LocationService *svc);
	static void removeService(LocationService *svc);

private slots:
	void gotFound(LocationService *svc, const QByteArray &id,
			const QList<Endpoint> &addrs);
	void gotLocateDone(LocationService *svc, const QByteArray &id);
};


// Virtual base class for services providing location information
class LocationService : public QObject
{
	friend class Locator;
	Q_OBJECT

public:
	LocationService(QObject *parent);
	virtual ~LocationService();

	// The subclass provides this method, to start an asynchronous search
	// for useable address information for a given host ID.
	virtual void locate(const QByteArray &id) = 0;

signals:
	// The subclass should emit this signal
	// when it finds one or more possible addresses for a desired host.
	void locateResult(LocationService *svc,
			const QByteArray &id, const QList<Endpoint> &addrs);

	// The subclass should emit this signal when its search
	// for a given ID has finished and no more results can be expected.
	void locateDone(LocationService *svc, const QByteArray &id);
};


#endif	// LOCATE_H
