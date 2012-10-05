//
// Congestion Control
//
#ifndef CC_H
#define CC_H

#include <QTime>

class Flow;


// Abstract base class for congestion control schemes.
class FlowCC
{
	friend class Flow;

protected:
	Flow *const flow;

	inline FlowCC(Flow *flow) : flow(flow) { }


	// The Flow calls this method when the sequence number jumps forward,
	// which usually means the missed packets were lost
	// (although occasionally they might just be out of order).
	// This method in effect handles "packet loss events".
	virtual void packetsMissed(int npackets);

	// The Flow calls this method when it receives acknowledgments
	// for one or more packets not previously acknowledged.
	virtual void packetsAcked(int npackets);

	// The Flow calls this method once each round-trip,
	// after updating the Flow's statistics such as round-trip time.
	void FlowCC::roundTrip(int rtt, int marksent, int markacks);

	// This method represents the crux of congestion control:
	// it returns how many new packets we may transmit right now.
	virtual int mayTransmit() = 0;

	virtual ~FlowCC();
};

// Classic "TCP Reno"-style congestion controller.
// Other CC algorithms often build on this.
class RenoCC : public FlowCC
{
	// Minimum and maximum congestion window sizes (in packets/RTT)
	static const quint32 cwndMin = 2;
	static const quint32 cwndMax = 1 << 20;

	quint32 ssthresh;	// Slow start threshold
	quint32 cwnd;		// Congestion window

	virtual void packetsMissed(int npackets);
	virtual void packetsAcked(int npackets);
	virtual void roundTrip();
	virtual int mayTransmit();

	// Set a new congestion window, clamping it against CWND_MAX,
	// and also don't let it exceed twice the actual
	// current number of outstanding packets in the network.
	// This can happen if our actual transmission rate
	// remains lower than the network's capacity for some period of time.
	int setCwnd(int cwnd);

public:
	inline RenoCC(Flow *flow) : FlowCC(flow) { }
};

// "TCP Vegas"-style congestion control
// (though not quite the exact Vegas algorithm)
class VegasCC : public RenoCC
{
	float basertt;
	float cwndmax;

	virtual void packetsAcked(int npackets);

public:
	inline VegasCC(Flow *flow) : RenoCC(flow) { }
};

// Experimental power-optimizing congestion control algorithm
class PowerCC : public BasicCC
{
	// Aggressive congestion control
	quint32 ssbase;		// Slow start baseline

	// Low-delay congestion control
	int cwndinc;
	int lastrtt;		// Measured RTT of last round-trip
	float lastpps;		// Measured PPS of last round-trip
	quint32 basewnd;
	float basertt, basepps, basepwr;

	virtual void packetsMissed(int npackets);
	virtual void packetsAcked(int npackets);

public:
	inline PowerCC(Flow *flow) : BasicCC(flow) { }
};


#endif	// CC_H
