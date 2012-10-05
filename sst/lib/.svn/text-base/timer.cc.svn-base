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


#include <QDateTime>
#include <QTimerEvent>
#include <QtDebug>

#include "timer.h"
#include "xdr.h"

using namespace SST;


////////// Time //////////

QString Time::toString() const
{
	return toQDateTime().toString();
}

QDateTime Time::toQDateTime() const
{
	QDateTime qdt;
	qdt.setTime_t(usecs / 1000000);
	qdt.addMSecs((usecs / 1000) % 1000);
	return qdt;
}

Time Time::fromString(const QString &str)
{
	return fromQDateTime(QDateTime::fromString(str));
}

Time Time::fromQDateTime(const QDateTime &qdt)
{
	// Qt's QDateTime only gives us millisecond accuracy; oh well...
	Time t;
	t.usecs = (qint64)qdt.toTime_t() * 1000000
		+ qdt.time().msec() * 1000;
	return t;
}

XdrStream &SST::operator>>(XdrStream &rs, Time &t)
{
	return rs >> t.usecs;
}

XdrStream &SST::operator<<(XdrStream &ws, const Time &t)
{
	return ws << t.usecs;
}

QByteArray Time::encode() const
{
	QByteArray buf;
	XdrStream ws(&buf, QIODevice::WriteOnly);
	ws << *this;
	return buf;
}

Time Time::decode(const QByteArray &data)
{
	XdrStream rs(data);
	Time t;
	rs >> t;
	return t;
}


////////// TimerEngine //////////

TimerEngine::TimerEngine(Timer *t)
:	QObject(t)
{
}

void TimerEngine::timeout()
{
	Timer *t = timer();
	t->timeout((t->fail -= t->iv) <= 0);
}


////////// Timer //////////

const qint64 Timer::retryMin;
const qint64 Timer::retryMax;
const qint64 Timer::failMax;

Timer::Timer(TimerHostState *host, QObject *parent)
:	QObject(parent),
	iv(retryMin),
	fail(failMax),
	act(false)
{
	te = host->newTimerEngine(this);
}


////////// DefaultTimerEngine //////////

DefaultTimerEngine::~DefaultTimerEngine()
{
	stop();
}

void DefaultTimerEngine::start(quint64 usecs)
{
	if (timerid)
		QObject::killTimer(timerid);
	timerid = QObject::startTimer(usecs / 1000);
	//qDebug() << "startTimer" << usecs << "id" << timerid
	//	<< "at" << QTime::currentTime().msec();
}

void DefaultTimerEngine::stop()
{
	//qDebug() << "stop at" << QTime::currentTime().msec();
	if (timerid) {
		QObject::killTimer(timerid);
		timerid = 0;
	}
}

void DefaultTimerEngine::timerEvent(QTimerEvent *ev)
{
	//qDebug() << "timerEvent id" << ev->timerId()
	//	<< "at" << QTime::currentTime().msec();
	if (timerid != 0 && ev->timerId() == timerid) {
		QObject::killTimer(timerid);
		timerid = 0;
		timeout();
	}
}


////////// TimerHostState //////////

Time TimerHostState::currentTime()
{
	return Time::fromQDateTime(QDateTime::currentDateTime());
}

TimerEngine *TimerHostState::newTimerEngine(Timer *timer)
{
	return new DefaultTimerEngine(timer);
}

