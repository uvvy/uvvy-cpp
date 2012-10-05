
#include <sst/host.h>

#include "capi/event.h"

using namespace SST;


////////// EventDispatcher //////////

EventDispatcher::EventDispatcher(QObject *parent)
:	QAbstractEventDispatcher(parent)
{
}

void EventDispatcher::flush()
{
	// Do nothing.
	// (XX is there anything we SHOULD do?
	// Does it make sense to expose this to the client application?)
}

bool EventDispatcher::hasPendingEvents()
{
	Q_ASSERT(0 /* XX not implemented */);
	return false;
}

bool EventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
	Q_ASSERT(0 /* XX not implemented */);
	return false;
}

void EventDispatcher::interrupt()
{
	Q_ASSERT(0 /* XX not implemented */);
}

void EventDispatcher::registerSocketNotifier(QSocketNotifier *notifier)
{
	Q_ASSERT(notifier != NULL);

	int fd = notifier->socket();
	Q_ASSERT(fd >= 0);

	int type = notifier->type();
	Q_ASSERT(type >= 0 && type <= 2);

	// Make sure our SockRec array covers this file descriptor */
	while (sockrecs.size() <= fd)
		sockrecs.append(SockRec());

	// Register this QSocketNotifier
	SockRec &rec = sockrecs[fd];
	if (rec.sn[type] != NULL)
		qWarning("SST::EventDispatcher: multiple socket notifiers "
			"for socket %d and type %d", fd, type);
	rec.sn[type] = notifier;

	// Update the client
	selectupdate(fd);
}

void EventDispatcher::unregisterSocketNotifier(QSocketNotifier *notifier)
{
	Q_ASSERT(notifier != NULL);

	int fd = notifier->socket();
	Q_ASSERT(fd >= 0);

	int type = notifier->type();
	Q_ASSERT(type >= 0 && type <= 2);

	// Our SockRec array should contain this notifier.
	Q_ASSERT(sockrecs.size() > fd);
	SockRec &rec = sockrecs[fd];
	if (rc.sn[type] != notifier)
		qWarning("SST::EventDispatcher: unregistering wrong notifier");
	rec.sn[type] = NULL;

	// Update the client
	selectupdate(fd);
}

void EventDispatcher::selectupdate(int fd)
{
	SockRec &rec = sockrecs[fd];

	sst_selectmask mask = 0;
	if (rec.sn[0])
		mask |= sst_selectread;
	if (rec.sn[1])
		mask |= sst_selectwrite;
	if (rec.sn[2])
		mask |= sst_selectexcept;

	// Invoke the client's callback with the new mask
	selectcb(selectcbdata, fd, mask);
}

void EventDispatcher::registerTimer(int timerid, int interval, QObject *object)
{
	Q_ASSERT(timerId > 0);
	Q_ASSERT(interval >= 0);
	Q_ASSERT(object != NULL);

	// Create the TimerRec
	TimerRec tr;
	tr.timerid = timerid;
	tr.interval = interval;
	tr.object = object;
	tr.waketime = currentTime().usecs + (qint64)interval * 1000;

	// Insert it in the appropriate place
	int i = 0;
	while (i < timerecs.size() && timerecs[i].waketime <= tr.waketime)
		i++;
	timerecs.insert(i, tr);
}

bool EventDispatcher::unregisterTimer(int timerid)
{
	Q_ASSERT(timerId > 0);

	for (int i = 0; i < timerecs.size(); i++) {
		if (timerecs[i].timerid == timerid) {
			timerecs.removeAt(i);
			return true;
		}
	}
	return false;
}

bool EventDispatcher::unregisterTimers(QObject *object)
{
	Q_ASSERT(object != NULL);

	bool found = false;
	for (int i = timerecs.size()-1; i >= 0; i--) {
		if (timerecs[i].object == object) {
			timerecs.removeAt(i);
			found = true;
		}
	}
	return found;
}

QList<TimerInfo> EventDispatcher::registeredTimers(QObject *object) const
{
	QList<TimerInfo> til;
	for (int i = 0; i < timerecs.size(); i++) {
		TimeRec &tr = timerecs[i];
		if (tr.object == object)
			til << TimerInfo(tr.timerid, tr.interval);
	}
	return til;
}

void EventDispatcher::wakeUp()
{
	Q_ASSERT(0 /* XX not implemented */);
}

