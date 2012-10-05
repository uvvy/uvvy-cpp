#ifndef SST_CAPI_EVENT_H
#define SST_CAPI_EVENT_H

#include <QAbstractEventDispatcher>

class sst_host;


/** This class reimplements Qt's QAbstractEventDispatcher
 * to give arbitrary C/C++ client applications an easy way
 * to integrate SST's Qt-based event system into the application's.
 */
class EventDispatcher : public QAbstractEventDispatcher
{
	Q_OBJECT

private:
	// Socket notifier records
	struct SockRec {
		QSocketNotifier *sn[3];	// cur socket notifier of each type

		inline SockRec() { sn[0] = NULL; sn[1] = NULL; sn[2] = NULL; }
	};
	QList<SockRec> sockrecs;	// List of sockrecs by fd number

	// Timer records
#if 0
	struct TimeRec {
		int timerid;		// timerId of this timer
		int interval;		// original timer interval in msecs
		QObject *object;	// object to be notified
		quint64 waketime;	// next absolute wakeup time in usecs
	};
#endif
	QList<sst_timerec> timerecs;	// Active timeouts in order of expiry

public:
	EventDispatcher(QObject *parent);

	virtual void flush();
	virtual bool hasPendingEvents();
	virtual void interrupt();
	virtual bool processEvents(QEventLoop::ProcessEventsFlags flags);
	virtual void registerSocketNotifier(QSocketNotifier * notifier);
	virtual void registerTimer(int timerId, int interval, QObject * object);
	virtual QList<TimerInfo> registeredTimers(QObject * object) const;
	virtual void unregisterSocketNotifier(QSocketNotifier * notifier);
	virtual bool unregisterTimer(int timerId);
	virtual bool unregisterTimers(QObject * object);
	virtual void wakeUp();

	// Called by sst_host_processevents()
	// to process events using the custom event dispatcher.
	int processevents(const fd_set *readfds, const fd_set *writefds,
			const fd_set *exceptfds, timeval *maxtime);
};


#endif	// SST_CAPI_EVENT_H
