
#include <sys/select.h>

#include "capi/chost.h"


////////// C functions comprising the public API //////////

sst_host *sst_host_create()
{
	return new sst_host();
}

void sst_host_free(sst_host *host)
{
	delete host;
}

// Register a time callback to virtualize the system time
void sst_host_timecallback(
		sst_host *host,
		void (*timecb)(void *cbdata, struct timeval *tp),
		void *cbdata)
{
	host->timecallback(timecb, cbdata);
}

// Instantiate our custom EventDispatcher class
// and attach it to the current thread.
void sst_host_selectcallback(
		sst_host *host,
		void (*selectcb)(void *cbdata, int fd, sst_selectmask mask),
		void *cbdata)
{
	host->selectcallback(selectcb, cbdata);
}

int sst_host_processevents(
		sst_host *host, const fd_set *readfds, const fd_set *writefds,
		const fd_set *exceptfds, timeval *maxtime)
{
	// Client must only call this if it has registered a select callback.
	Q_ASSERT(host->evdisp != NULL);

	// Process events via the event dispatcher
	return evdisp->processevents(readfds, writefds, exceptfds, maxtime);
}


////////// Class sst_host (C++ only) //////////

sst_host::sst_host()
:	selectcb(NULL), evdisp(NULL),
	timecb(dfltimecb)
{
}

void sst_host::timecallback(void (*timecb)(void *, struct timeval *),
			void *cbdata)
{
	// Client must only call this function once per sst_host instance
	Q_ASSERT(this->timecb == dfltimecb);

	this->timecb = timecb;
	this->timecbdata = cbdata;
}

void sst_host::selectcallback(void (*selectcb)(void *, int, sst_selectmask),
			void *cbdata)
{
	// Client must only call this function once per sst_host instance
	Q_ASSERT(this->selectcb == NULL);

	// Must not already have an event dispatcher registered on this thread
	Q_ASSERT(QAbstractEventDispatcher::instance() == NULL);

	// Register the callback
	this->selectcb = selectcb;
	this->selectcbdata = cbdata;

	// Create the event dispatcher
	(void)new EventDispatcher(host);
}

void sst_host::dfltimecb(void *cbdata, struct timeval *tv)
{
	if (gettimeofday(&tv, NULL) < 0)
		qWarning("sst_host: gettimeofday failed");
}

Time sst_host::currentTime()
{
	// Get the system time via the appropriate (or default) callback
	timeval tv;
	timecb(timecbdata, &tv);

	// Return it as a 64-bit SST timestamp.
	return Time((qint64)tv.sec * 1000000 + tv.tv_usec);
}

TimerEngine *newTimerEngine(Timer *timer)
{
	// If the client application is using a custom event loop,
	// timer processing has to use it too.
	// Otherwise, stick with native Qt timers by default.
	if (evdisp)
		return evdisp->newTimerEngine(timer);
	else
		return SST::Host::newTimerEngine(timer);
}

