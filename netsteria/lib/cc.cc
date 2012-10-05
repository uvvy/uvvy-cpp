
#include <algorithm>

#include "cc.h"
#include "flow.h"


////////// FlowCC //////////

void FlowCC::packetsMissed(int newpackets)
{
}

void FlowCC::packetsAcked(int newpackets)
{
}

void FlowCC::roundTrip(int rtt, int marksent, int markacks)
{
}


////////// RenoCC //////////

int RenoCC::mayTransmit()
{
	int unacked = flow->unackedDataPackets();
	if (cwnd > unacked)
		return cwnd - unacked;
	else
		return 0;
}

int RenoCC:setCwnd(int cwnd)
{
	// Clamp cwnd against CWND_MIN and CWND_MAX,
	// and also never let it exceed twice the actual
	// current number of outstanding packets in the network.
	// This can happen if our actual transmission rate
	// remains lower than the network's capacity for some period of time.
	cwnd = std::max(cwnd, CWND_MIN);
	cwnd = std::min(cwnd, CWND_MAX);
	cwnd = std::min(cwnd, (unsigned)flow->unackedPackets() * 2);
	this->cwnd = cwnd;
}

void RenoCC::packetsMissed(int npackets)
{
	qDebug("%d PACKETS LOST: cwnd %d -> %d",
		ackdiff - newpackets, cwnd, ssthresh);

	// Packet loss detected -
	// perform standard TCP congestion control.
	// (XX once per npackets?)
	ssthresh = flow->unackedPackets() / 2;
	ssthresh = std::max(ssthresh, CWND_MIN);
	setCwnd(ssthresh);
}

void RenoCC::packetsAcked(int newpackets)
{
	// Count acknowledged packets
	BasicCC::packetsAcked(newpackets);

	// During the standard TCP slow start procedure,
	// increment cwnd for each newly-ACKed packet.
	// XX TCP spec says this should be <=,
	// but that puts us in slow start briefly after every loss!
	if (cwnd < ssthresh) {
		setCwnd(cwnd + newpackets);
		qDebug("Slow start: %d new ACKs; boost cwnd to %d",
			newpackets, cwnd);
	}
}

void RenoCC::roundTrip(int rtt, int marksent, int markacks)
{
	// Normal TCP congestion control:
	// Increment cwnd once each RTT during congestion avoidance.
	setCwnd(cwnd + 1);
}

////////// VegasCC //////////

void VegasCC::roundTrip(int rtt, int marksent, int markacks)
{
	if (rtt < basertt)
		basertt = rtt;
	else
		basertt = (basertt * 255.0 + rtt) / 256.0;

	float expect = (float)marksent / basertt;
	float actual = (float)marksent / rtt;
	float diffpps = expect - actual;
	Q_ASSERT(diffpps >= 0.0);
	float diffpprt = diffpps * rtt;

	if (diffpprt < 1.0 && cwnd < CWND_MAX)
		setCwnd(cwnd + 1);
	else if (diffpprt > 3.0 && cwnd > CWND_MIN)
		setCwnd(cwnd - 1);

//	qDebug("Round-trip: basertt %.3f rtt %d "
//		"exp %.3f act %.3f diff %.3f cwnd %d",
//		basertt, rtt, expect, actual, diffpprt, cwnd);
}

////////// PowerCC //////////

void PowerCC::packetsMissed(int npackets)
{
	// Number of packets we think have been lost
	// so far during this round-trip.
	int lost = (flow->txackseq - markbase) - markacks;
	lost = std::max(0, lost);

	// Number of packets we expect to receive,
	// assuming the lost ones are really lost
	// and we don't lose any more this round-trip.
	unsigned expected = marksent - lost;

	// Clamp the congestion window to this value.
	if (expected < cwnd) {
		qDebug("%d PACKETS LOST: cwnd %d -> %d",
			ackdiff - newpackets,
			cwnd, expected);
		cwnd = ssbase = expected;
		cwnd = std::max(CWND_MIN, cwnd);
	}
}

void PowerCC::packetsAcked(int newpackets)
{
	// Count acknowledged packets
	BasicCC::packetsAcked(newpackets);

	if (cwndinc < 0)	// Only slow start during up-phase
		return;

	// We're always in slow start, but we only count ACKs received
	// on schedule and after a per-roundtrip baseline.
	if (markacks > ssbase && marktime.elapsed() <= lastrtt) {
		cwnd += std::min(newpackets, markacks - ssbase);
	//	qDebug("Slow start: %d new ACKs; boost cwnd to %d",
	//		newpackets, cwnd);
	}
}

void FlowCC::roundTrip(int rtt, int marksent, int markacks)
{
	if (pwr > basepwr) {
		basepwr = pwr;
		basertt = rtt;
		basepps = pps;
		basewnd = markacks;
	} else if (markacks <= basewnd && rtt > basertt) {
		basertt = rtt;
		basepwr = basepps / basertt;
	} else if (markacks >= basewnd && pps < basepps) {
		basepps = pps;
		basepwr = basepps / basertt;
	}

	if (cwndinc > 0) {
		// Window going up.
		// If RTT makes a significant jump, reverse.
		if (rtt > basertt || cwnd >= CWND_MAX) {
			cwndinc = -1;
		} else {
			// Additively increase the window
			cwnd += cwndinc;
		}
	} else {
		// Window going down.
		// If PPS makes a significant dive, reverse.
		if (pps < basepps || cwnd <= CWND_MIN) {
			ssbase = cwnd++;
			cwndinc = +1;
		} else {
			// Additively decrease the window
			cwnd += cwndinc;
		}
	}
	cwnd = std::max(CWND_MIN, cwnd);
	cwnd = std::min(CWND_MAX, cwnd);
	qDebug("RT: pwr %.0f[%.0f/%.0f]@%d "
		"base %.0f[%.0f/%.0f]@%d "
		"cwnd %d%+d",
		pwr*1000.0, pps, (float)rtt, markacks,
		basepwr*1000.0, basepps, basertt, basewnd,
		cwnd, cwndinc);
	break;
}

