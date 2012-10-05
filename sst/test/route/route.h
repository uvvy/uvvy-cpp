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
#ifndef ROUTE_H
#define ROUTE_H

#include <QObject>
#include <QByteArray>
#include <QtDebug>

#include "timer.h"
#include "sock.h"
#include "host.h"

namespace SST {


static const quint32 routerMagic = 0x00525452;	// 'RTR'

typedef QByteArray NodeId;


// Compute the affinity between two NodeIds - i.e., the first different bit.
int affinity(const SST::NodeId &a, const SST::NodeId &b);


// One hop on a path
struct Hop
{
	quint32 rid;		// Local routing identifier
	NodeId nid;		// Full node identifier

	inline Hop() : rid(0) { }
	inline Hop(quint32 rid, NodeId nid)
		: rid(rid), nid(nid) { }

	inline bool operator==(const Hop &other) const
		{ return rid == other.rid && nid == other.nid; }
	inline bool operator!=(const Hop &other) const
		{ return rid != other.rid || nid != other.nid; }
};

// A particular path to some node, with the last known state of that path
struct Path
{
	NodeId start;		// Node at which path begins
	QList<Hop> hops;	// List of routing hops comprising  path
	double weight;		// Measured weight/distance/latency of path
	Time stamp;		// Time path was discovered or last checked

	inline Path() : weight(0) { }
	inline Path(const NodeId &start) : start(start), weight(0) { }
	inline Path(const NodeId &start, quint32 rid, NodeId nid,
			double weight)
		: start(start), weight(weight) { hops.append(Hop(rid, nid)); }

	// A null Path has no starting node Id
	inline bool isNull() const { return start.isEmpty(); }

	// An empty path has no hops
	inline bool isEmpty() const { return hops.isEmpty(); }
	inline int numHops() const { return hops.size(); }

	inline NodeId originId() const { return start; }
	inline NodeId targetId() const
		{ return hops.isEmpty() ? start : hops.last().nid; }

	// Return the node ID just before or just after a given hop
	inline NodeId beforeHopId(int hopno) const
		{ return hopno == 0 ? start : hops.at(hopno-1).nid; }
	inline NodeId afterHopId(int hopno) const
		{ return hops.at(hopno).nid; }

	/// Return the number of hops to reach a given node,
	/// 0 if the node is our starting point, -1 if it is not on the path.
	int hopsBefore(const NodeId &nid) const;

	/// Append a new hop at the end of this path.
	void append(quint32 rid, NodeId nid, double hopWeight)
		{ hops.append(Hop(rid, nid)); weight += hopWeight; }

	/// Prepend a hop to the path, changing the starting node.
	void prepend(NodeId nid, quint32 rid, double hopWeight)
		{ hops.prepend(Hop(rid, start)); start = nid;
		  weight += hopWeight; }

	inline void removeFirst() { start = afterHopId(0); hops.removeFirst(); }
	inline void removeLast() { hops.removeLast(); }

	/// Extend this path by appending another Path onto the tail.
	Path &operator+=(const Path &tail);

	inline Path operator+(const Path &tail) const
		{ Path pi(*this); pi += tail; return pi; }

	// Test the route for loops and return true if there are any.
	bool looping() const;
};

// A node's information about some other node it keeps tabs on
//class Peer
//{
//private:
//	int refcount;
//
//public:
//	Peer();
//	inline void take()
//		{ refcount++; Q_ASSERT(refcount > 0); }
//	inline void drop()
//		{ Q_ASSERT(refcount > 0); if (--refcount == 0) delete this; }
//};

class Bucket
{
public:
	// Common info
	QHash<NodeId,Path> peers; // Best paths to other nodes at this level

	// Slave info
	QList<Path> paths;	// Best paths to closest master at this level
	QHash<NodeId,Path> mps;	// Best master path received from each peer
	QHash<NodeId,Path> nbs; // Best paths to masters of other neighborhoods


	inline Path masterPath() const
		{ return paths.isEmpty() ? Path() : paths.at(0); }
	inline NodeId masterId() const
		{ return masterPath().targetId(); }

	/// Insert a newly-discovered Path into this bucket.
	/// Returns true if the new Path was actually accepted:
	/// i.e., if the bucket wasn't already full of "better" paths.
	bool insert(const Path &path);

	/// Return our current internal distance horizon for this bucket:
	/// i.e., the weight beyond which we don't need more nodes.
	/// Returns +infinity if the bucket isn't full yet.
	double horizon() const;
};

class Router : public SocketReceiver
{
	Q_OBJECT

public:
	Router(Host *h, const QByteArray &id, QObject *parent = NULL);

	const QByteArray id;		// pseudorandom node label

	QList<Bucket> buckets;


	inline int affinityWith(const NodeId &otherId) const
		{ return affinity(id, otherId); }

	inline Bucket &bucket(int aff)
		{ while (buckets.size() <= aff) buckets.append(Bucket());
		  return buckets[aff]; }
	inline Bucket &bucket(const NodeId &nid)
		{ return bucket(affinityWith(nid)); }

	inline bool insertPath(const Path &p)
		{ return bucket(p.targetId()).insert(p); }

	// Return the current best path to a given node.
	Path pathTo(const NodeId &id) const;

	// Find the best known path to our nearest neighbor
	// with higher affinity with 'id' than us.
	Path nearestNeighborPath(const NodeId &id) const;

	// Return the ID of the node in our neighbor tables nearest to 'id'.
	inline NodeId nearestNeighbor(const NodeId &id) const
		{ return nearestNeighborPath(id).targetId(); }

	// Find up to 'maxpaths' paths to neighbors with high affinity to 'id'.
	// If the 'paths' list is nonempty on entry to this method,
	// new paths are merged into the list incrementally.
	void nearestNeighborPaths(const NodeId &id, QList<Path> &paths,
				int maxpaths) const;

	virtual void receive(QByteArray &msg, XdrStream &ds,
				const SocketEndpoint &src);
};

} // namespace SST

QDebug operator<<(QDebug debug, const SST::Path &p);


#endif	// ROUTE_H
