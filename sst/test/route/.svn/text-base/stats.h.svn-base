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
#ifndef STATS_H
#define STATS_H

#include <QList>

class QDebug;

/// Useful class for standard statistical computations:
/// min, max, mean, median, standard deviation.
class Stats
{
public:
	enum Type {
		Population = 1,
		Sample = 2,
	};

private:
	const Type type;
	QList<double> vals;
	double min, max;
	double accum;
	bool sorted;		// true if vals array is sorted

public:
	inline Stats(Type type)
		: type(type), min(HUGE_VAL), max(-HUGE_VAL), accum(0),
		  sorted(false) { }

	/// Insert a value into the stats pool
	void insert(double value);

	/// Clear the stats pool in preparation for a new set of values
	void clear();

	/// Returns the number of values we've inserted so far
	inline int count() const { return vals.size(); }

	/// Returns the minimum of all the values inserted so far
	inline double minimum() const { return min; }

	/// Returns the maximum of all the values inserted so far
	inline double maximum() const { return max; }

	/// Returns the mean (average) of all values inserted so far
	inline double mean() const { return accum / count(); }
	inline double average() const { return mean(); }

	/// Returns the median (middle value)
	double median() const;

	/// Compute standard deviation according to type
	double standardDeviation() const;

private:
	inline void sort() { if (!sorted) { qSort(vals); sorted = true; } }

	double variance() const;	// Compute total variance
};

class SampleStats : public Stats
{
public:
	SampleStats() : Stats(Sample) { }
};

class PopStats : public Stats
{
public:
	PopStats() : Stats(Population) { }
};

QDebug &operator<<(QDebug &debug, const Stats &s);

#endif	// STATS_H
