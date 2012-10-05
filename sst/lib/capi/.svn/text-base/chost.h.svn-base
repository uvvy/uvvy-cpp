#ifndef SST_CAPI_CHOST_H
#define SST_CAPI_CHOST_H

#include <QList>

#include <sst/host.h>

#include "host.h"
#include "timer.h"

class QSocketNotifier;

class sst_host;
class sst_eventdispatcher;


/** TimerEngine subclass for SST's C-exported API.
 */
struct sst_timerec : public SST::TimerEngine
{
	friend class sst_host;
	Q_OBJECT

private:
	qint64 waketime;	// Absolute wakeup time, 0 if inactive

	inline sst_timerec(Timer *t) : TimerEngine(t), timerid(0) { }
	~sst_timerec();

	virtual void start(quint64 usecs);
	virtual void stop();
};


/** SST Host instance class as exported via the C API.
 * Client applications can't see the definition of this "struct"
 * (it's totally opaque), so it's OK that it's really a C++ class.
 */
struct sst_host : public SST::Host
{
	Q_OBJECT

private:
	// Custom event dispatcher state
	void (*selectcb)(void *cbdata, int fd, sst_selectmask mask);
	void *selectcbdata;
	sst_eventdispatcher *evdisp;

	// System time virtualization state
	void (*timecb)(void *cbdata, struct timeval *tp),
	void *timecbdata;

public:
	sst_host();

	void timecallback(void (*timecb)(void *, struct timeval *),
				void *cbdata);
	void selectcallback(void (*selectcb)(void *, int, sst_selectmask),
				void *cbdata);

	// Overriden methods of TimerHostState
	Time currentTime();
	TimerEngine *newTimerEngine(Timer *timer);

private:
	void selectupdate(int fd);

	static void dfltimecb(void *cbdata, struct timeval *tv);
};


#endif	// SST_CAPI_CHOST_H
