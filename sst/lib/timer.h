/*
 * Structured Stream Transport
 * Copyright (C) 2006-2008 Massachusetts Institute of Technology
 * Author: Bryan Ford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
// Timing and timers for SST protocol use.
#ifndef SST_TIMER_H
#define SST_TIMER_H

#include <QObject>

class QDateTime;
class QTimerEvent;


namespace SST {

class XdrStream;
class Timer;
class TimerEngine;
class TimerHostState;


/** 64-bit time class convenient for protocol use.
 * Represents in a simple, easy-to-work-with 64-bit time format
 * by counting microseconds since the Unix epoch.
 * The current() method to check the system time
 * supports time virtualization for protocol simulation purposes. */
class Time
{
	friend class Timer;

public:
	qint64 usecs;		///< Microseconds since the Unix Epoch


	inline Time() { }
	inline Time(qint64 usecs) : usecs(usecs) { }
	inline Time(const Time &other) : usecs(other.usecs) { }

	inline bool operator==(const Time &other) const
		{ return usecs == other.usecs; }
	inline bool operator!=(const Time &other) const
		{ return usecs != other.usecs; }
	inline bool operator<(const Time &other) const
		{ return usecs < other.usecs; }
	inline bool operator>(const Time &other) const
		{ return usecs > other.usecs; }
	inline bool operator<=(const Time &other) const
		{ return usecs <= other.usecs; }
	inline bool operator>=(const Time &other) const
		{ return usecs >= other.usecs; }


	/** Find the amount time since some supposedly previous timestamp,
	 * or 0 if the 'previous' timestamp is actually later. */
	inline Time since(const Time &other) const
		{ return Time(qMax(usecs - other.usecs, (qint64)0)); }


	/** Convert to standard ASCII date/time representation.
	 * Uses Qt's QDateTime formatting mechanism.
	 * @return the resulting date/time string. */
	QString toString() const;

	/** Convert from standard ASCII date/time representations.
	 * @param str a date/time string in any of the formats
	 *	supported by Qt's QDateTime class.
	 * @return the resulting Time. */
	static Time fromString(const QString &str);


	/** Convert to Qt's QDateTime class.
	 * @return the resulting QDateTime. */
	QDateTime toQDateTime() const;

	/** Convert from Qt's QDateTime class.
	 * @param qdt the QDateTime to convert.
	 * @return the resulting Time. */
	static Time fromQDateTime(const QDateTime &qdt);


	// XX actually need these?
	static Time decode(const QByteArray &data);
	QByteArray encode() const;
};

XdrStream &operator>>(XdrStream &rs, Time &t);
XdrStream &operator<<(XdrStream &ws, const Time &t);


/** Abstract base class for timer implementations.
 */
class TimerEngine : public QObject
{
	friend class Timer;

	Q_OBJECT

protected:
	/** Create a new TimerEngine */
	TimerEngine(Timer *t);

	/** Find the Timer with which this TimerEngine is associated.
	 * @return the Timer. */
	Timer *timer() const { return (Timer*)parent(); }


	/** Start the timer.
	 * The implementation subclass provides this method.
	 * @param usecs the timer interval in microseconds.
	 */
	virtual void start(quint64 usecs) = 0;

	/** Stop the timer.
	 * The implementation subclass provides this method.
	 */
	virtual void stop() = 0;


	/** Signal a timeout on the timer.
	 * The implementation subclass calls this method
	 * when the timer expires.
	 */
	void timeout();
};


/** Class implementing timers suitable for network protocols.
 * Supports exponential backoff computations
 * for retransmissions and retries,
 * as well as an optional "hard" failure deadline.
 * Also can be hooked and virtualized for simulation purposes.
 */
class Timer : public QObject
{
	friend class TimerEngine;

	Q_OBJECT

	TimerEngine *te;
	qint64 iv;
	qint64 fail;
	bool act;

public:

	/// Default initial retry time: 500 ms
	static const qint64 retryMin = 500*1000;

	/// Default max retry period: 1 minute
	static const qint64 retryMax = 60*1000*1000;

	/// Default hard failure deadline: 20 seconds
	static const qint64 failMax = 20*1000*1000;


	/// Exponential backoff function for retry.
	static qint64 backoff(qint64 period, qint64 maxperiod = failMax)
		{ return qMin(period * 3 / 2, maxperiod); }


	/** Create a timer.
	 * @param parent the optional parent for the new Timer. */
	Timer(TimerHostState *host, QObject *parent = NULL);

	/** Determine if the timer is currently active.
	 * @return true if the timer is ticking. */
	inline bool isActive() const { return act; }

	/* Obtain the timer's current interval.
	 * @return the current interval in microseconds */
	inline qint64 interval() const { return iv; }


	/** Start or restart the timer at a specified or default interval.
	 * @param initperiod the initial timer interval in microseconds.
	 * @param failperiod the hard failure interval in microseconds. */
	inline void start(qint64 initperiod = retryMin,
			  qint64 failperiod = failMax)
		{ fail = failperiod; act = true; te->start(iv = initperiod); }

	/** Stop the timer if it is currently running. */
	inline void stop()
		{ te->stop(); act = false; }

	/** Restart the timer with a longer interval after a retry. */
	inline void restart()
		{ act = true; te->start(iv = backoff(iv)); }

	/** Determine if we've reached the hard failure deadline.
	 * @return true if the failure deadline has passed. */
	inline bool failed()
		{ return fail <= 0; }


signals:
	/** Signaled when the timer expires.
	 * @param failure true if the hard failure deadline has been reached.
	 */
	void timeout(bool failed);
};


/** @internal
 * @brief Default TimerEngine implementation based on QObject's timers.
 */
class DefaultTimerEngine : public TimerEngine
{
	friend class TimerHostState;

	Q_OBJECT
	int timerid;

	inline DefaultTimerEngine(Timer *t) : TimerEngine(t), timerid(0) { }
	~DefaultTimerEngine();

	virtual void start(quint64 usecs);
	virtual void stop();

	/// Override QObject's timerEvent() method.
	virtual void timerEvent(QTimerEvent *);
};


/** Abstract base class providing hooks for time virtualization.
 * The application may create a TimerHostState object
 * and activate it using Time::setTimerHostState().
 * The SST protocol will then call the time factory's methods
 * whenever it needs to obtain the current system time
 * or create timers. */
class TimerHostState : public QObject
{
	Q_OBJECT

public:
	/** Obtain the current system time.
	 * May be overridden to virtualize the system time.
	 * @return the current Time. */
	virtual Time currentTime();


	/** Create a TimerEngine.
	 * May be overridden to virtualize the behavior of timers.
	 * @param the Timer for which this TimerEngine is needed.
	 * @return the new TimerEngine. */
	virtual TimerEngine *newTimerEngine(Timer *timer);
};

} // namespace SST


#endif	// SST_TIMER_H
