#ifndef SST_C_HOST_H
#define SST_C_HOST_H

#include <sys/time.h>

typedef struct sst_host sst_host;

#if 0
typedef enum sst_hostflags {
} sst_hostflags;
#endif

typedef void *sst_hostdata;
typedef void *sst_timerid;


enum sst_selectmask {
	sst_selectread		= 1,	/* Select for reading */
	sst_selectwrite		= 2,	/* Select for writing */
	sst_selectexcept	= 4,	/* Select for exceptions */
};

struct sst_hostenv
{
	int flags;

	
};


/** Create an sst_host object, representing an SST protocol stack instance.
 * Each SST protocol stack instance is completely independent.
 * A typical application only needs one sst_host instance,
 * but the application may wish to create several independent instances
 * to simulate the behavior of a protocol across several nodes
 * from within a single application process.
 *
 * The SST protocol stack is currently NOT thread safe in any way:
 * once the application creates an sst_host on one thread,
 * the application MUST use the SAME thread in all subsequent calls
 * into the SST protocol stack that operate on this sst_host instance.
 * Since different sst_host instances are independent, however,
 * the application may run different sst_host instances on separate threads.
 *
 * @return the new sst_host instance, or NULL if an error occurred,
 *	in which case errno describes the error.
 */
sst_host *sst_host_create(void);

/** Destroy an sst_host object.
 * Implicitly destroys all streams and other state
 * associated with this particular sst_host instance.
 *
 * @param host the sst_host instance to be freed.
 */
void sst_host_free(sst_host *host);

/** Establish a time callback for SST to use to query the current time.
 * If the application calls this function,
 * the SST protocol stack will use the provided callback
 * instead of the standard gettimeofday() function
 * whenever it needs to query the current system time.
 * The application may use this mechanism to virtualize
 * the notion of time that the SST protocol stack uses,
 * for an event-based simulation for example.
 * Since each sst_host instance is fully independent,
 * the application may give each sst_host a different notion of time.
 * 
 * @param host the sst_host instance on which to set the callback.
 * @param timecb the time callback function to set.
 * @param cbdata an opaque pointer that SST passes to the timecb
 *		whenever it invokes the registered callback.
 */
void sst_host_timecallback(
		sst_host *host,
		void (*timecb)(void *cbdata, struct timeval *tp),
		void *cbdata);

/** Establish a select callback for SST event handling.
 * If the client links directly with the SST protocol stack,
 * but does not wish to use either the native Qt or Glib event loop,
 * it MUST call this function before starting to use the sst_host instance.
 * The client must provide a callback function (selectcb),
 * which SST calls whenever it needs to modify the set of file descriptors
 * that it needs to watch for interesting events.
 *
 * When SST invokes the callback,
 * it passes an opaque client-specified callback data pointer,
 * a file descriptor number, and a mask of event types to watch for:
 * any combination of sst_selectread, sst_selectwrite, and sst_selectexcept.
 * The application should use this file descriptor and mask
 * to adjust the fd_sets it passes to select() in its main loop.
 * SST invokes the callback with a mask of zero to indicate that
 * it no longer wishes to receive any events for the given file descriptor:
 * e.g., just before closing the file descriptor.
 *
 * The application's main loop should watch all the currently interesting
 * file descriptors as indicated by SST's calls to the callback function,
 * and call sst_host_processevents() to dispatch these events to SST.
 * The sst_host_processevents() function may also return a struct timeval
 * indicating the maximum time the application's main loop should wait
 * before calling sst_host_processevents() again:
 * the SST protocol stack uses this to implement timeouts.
 * These timeouts may be expressed in virtual time rather than system time,
 * however, if the application has registered a virtual-time callback
 * via sst_host_timecallback().
 *
 * The application may call sst_host_selectcallback()
 * only once on a particular sst_host instance.
 * This function is available only in libsst, not in libsststub.
 * This function is not available on Windows
 * (on which it is not needed anyway
 * because Windows has a standard main event loop).
 *
 * Internally, this function instantiates a custom subclass
 * of Qt's QAbstractEventDispatcher class
 * and attaches it to the current thread.
 * (This is why the application must always use the same thread
 * when calling into the SST protocol stack.)
 * SST's custom QAbstractEventDispatcher subclass supports
 * the event handling functionality required by the SST protocol stack,
 * but not all the functionality that arbitrary Qt-based code might need.
 * If the application also uses other Qt-based libraries or components
 * in addition to SST, therefore, the application may have to provide
 * its own full-featured QAbstractEventDispatcher
 * instead of calling this function.
 *
 * @param host the sst_host instance on which to set the callback.
 * @param selectcb the select callback function to set.
 * @param cbdata an opaque pointer that SST passes to the selectcb
 *		whenever it invokes the registered callback.
 */
void sst_host_selectcallback(
		sst_host *host,
		void (*selectcb)(void *cbdata, int fd, sst_selectmask mask),
		void *cbdata);

/** Check for and process any pending events for this sst_host instance.
 * The client application calls this function from its main event loop,
 * if it is not using either the Qt or Glib event loop,
 * and has called sst_host_selectcallback() to set a select callback.
 * SST processes any timeouts or events pending on its file descriptors,
 * then sets maxtime to the maximum amount of time the application should wait
 * before calling sst_host_processevents() again, and returns 1.
 * If no timeouts are pending within the given sst_host instance,
 * this function returns 0 without setting maxtime:
 * in this case the application only needs to call sst_host_processevents()
 * when any interesting events occur on SST's registered file descriptors.
 * It is safe for the application to make spurious calls
 * to sst_host_processevents() when no SST events or timeouts are pending.
 *
 * @param host the sst_host instance on which to process events.
 * @param readfds the set of file descriptors that may be ready for reading.
 * @param writefds the set of file descriptors that may be ready for writing.
 * @param exceptfds the set of file descriptors
		that may have pending exceptions.
 * @param maxtime a struct timeval which SST sets to the maximum time
 *		the application may wait before calling this function again.
 * @return 1 if SST timeouts are pending and maxtime is set accordingly,
 *		or 0 if no timeouts are pending and the application
 *		should ignore the contents of maxtime.
 */
int sst_host_processevents(
		sst_host *host, const fd_set *readfds, const fd_set *writefds,
		const fd_set *exceptfds, struct timeval *maxtime);

#endif	/* SST_C_HOST_H */
