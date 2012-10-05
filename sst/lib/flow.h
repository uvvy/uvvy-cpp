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
#ifndef SST_FLOW_H
#define SST_FLOW_H

#include <QTime>
#include <QTimer>
#include <QQueue>	// XXX FlowSegment

#include "ident.h"
#include "sock.h"
#include "timer.h"

// XXX for specific armor methods - break into separate module
#include "chk32.h"
#include "aes.h"
#include "hmac.h"


namespace SST {

class Host;
class Ident;
class Endpoint;
class Socket;
class FlowCC;
class KeyInitiator;	// XXX


// Packet sequence numbers are 64-bit unsigned integers
typedef quint64 PacketSeq;

static const PacketSeq maxPacketSeq = ~(PacketSeq)0;


enum CCMode {
	CC_TCP,
	CC_AGGRESSIVE,
	CC_DELAY,
	CC_VEGAS,
	CC_CTCP,
	CC_FIXED,
};


// Abstract base class for flow encryption and authentication schemes.
class FlowArmor : public QObject
{
	friend class Flow;
	Q_OBJECT

protected:
	// The pseudo-header is a header logically prepended to each packet
	// for authentication purposes but not actually transmitted.
	// XX probably can't always be static const
	//static const int phlen = 4;

	// The encryption offset defines the offset in the transmitted packet
	// at which encrypted data begins (after authenticate-only data).
	static const int encofs = 4;

	virtual QByteArray txenc(qint64 pktseq, const QByteArray &pkt) = 0;
	virtual bool rxdec(qint64 pktseq, QByteArray &pkt) = 0;

	virtual ~FlowArmor();
};

// Abstract base class representing a flow
// between a local Socket and a remote endpoint.
class Flow : public SocketFlow
{
	friend class KeyInitiator;	// XXX
	Q_OBJECT

private:
	Host *const h;
	FlowArmor *armr;	// Encryption/authentication method
	//FlowCC *cc;		// Congestion control method
	CCMode ccmode;		// Congestion control method
	bool nocc;		// Disable congestion control.  XXX
	unsigned missthresh;	// Threshold at which to infer packets dropped

	// Per-direction unique channel IDs for this channel.
	// Stream layer uses these in assigning USIDs to new streams.
	QByteArray txchanid;	// Transmit channel ID
	QByteArray rxchanid;	// Receive channel ID


public:
	// How many sequence numbers a packet needs to be out of date
	// before we consider it to be "missed", i.e., "probably dropped".
	// This is represents the "standard" Internet convention,
	// although (XXX) it really needs to be dynamically measured
	// to account for variable reordering rates on different paths.
	// XXX static const inst missdelay = 2;

	// Amount of space client must leave at the beginning of a packet
	// to be transmitted with flowTransmit() or received via flowReceive().
	// XX won't always be static const.
	static const int hdrlen = 8;

	// Layout of the first header word: channel number, tx sequence
	// Transmitted in cleartext.
	static const quint32 chanBits = 8;	// 31-24: channel number
	static const quint32 chanMask = (1 << chanBits) - 1;
	static const quint32 chanMax = chanMask;
	static const quint32 chanShift = 24;
	static const quint32 seqBits = 24;	// 23-0: tx sequence number
	static const quint32 seqMask = (1 << seqBits) - 1;

	// Layout of the second header word: ACK count/sequence number
	// Encrypted for transmission.
	static const quint32 resvBits = 4;	// 31-28: reserved field
	static const quint32 ackctBits = 4;	// 27-24: ack count
	static const quint32 ackctMask = (1 << ackctBits) - 1;
	static const quint32 ackctMax = ackctMask;
	static const quint32 ackctShift = 24;
	static const quint32 ackSeqBits = 24;	// 23-0: ack sequence number
	static const quint32 ackSeqMask = (1 << ackSeqBits) - 1;


	Flow(Host *host, QObject *parent = NULL);
	virtual ~Flow();

	inline Host *host() { return h; }

	// Set the encryption/authentication method for this flow.
	// This MUST be set before a new flow can be activated.
	inline void setArmor(FlowArmor *armor) { this->armr = armor; }
	inline FlowArmor *armor() { return armr; }

	// Set the channel IDs for this flow.
	inline QByteArray txChannelId() { return txchanid; }
	inline QByteArray rxChannelId() { return rxchanid; }
	inline void setChannelIds(const QByteArray &tx, const QByteArray &rx)
		{ txchanid = tx; rxchanid = rx; }

	// Set the congestion controller for this flow.
	// This must be set if the client wishes to call mayTransmit().
	//inline void setCongestionController(FlowCC *cc) { this->cc = cc; }
	//inline FlowCC *congestionController() { return cc; }

	// Start and stop the flow.
	virtual void start(bool initiator);
	virtual void stop();

	// Return the current link status as observed by this flow.
	inline LinkStatus linkStatus() const { return linkstat; }


protected:

	// Size of rxmask, rxackmask, and txackmask fields in bits
	static const int maskBits = 32;

	struct TxEvent {
		qint32	size;	// Total size of packet including hdr
		bool	data;	// Was an upper-layer data packet
		bool	pipe;	// Currently counted toward txdatpipe

		inline TxEvent(qint32 size, bool isdata)
			: size(size), data(isdata), pipe(isdata) { }
	};

	// Transmit state
	quint64 txseq;		// Next sequence number to transmit
	QQueue<TxEvent> txevts;	// Record of transmission events (XX data sizes)
	quint64 txevtseq;	// Seqno of oldest recorded tx event
	quint64 txackseq;	// Highest transmit sequence number ACK'd
	quint64 recovseq;	// Sequence at which fast recovery finishes
	quint64 markseq;	// Transmit sequence number of "marked" packet
	quint64 markbase;	// Snapshot of txackseq at time mark was placed
	Time marktime;		// Time at which marked packet was sent
	quint32 txackmask;	// Mask of packets transmitted and ACK'd
	quint32	txfltcnt;	// Data packets currently in flight
	quint32	txfltsize;	// Data bytes currently in flight
	quint32 markacks;	// Number of ACK'd packets since last mark
	quint32 marksent;	// Number of ACKs expected after last mark
	quint32 cwnd;		// Current congestion window
	bool cwndlim;		// We were cwnd-limited this round-trip

	// TCP congestion control
	quint32 ssthresh;	// Slow start threshold
	bool sstoggle;		// Slow start toggle flag for CC_VEGAS

	// Aggressive congestion control
	quint32 ssbase;		// Slow start baseline

	// Low-delay congestion control
	int cwndinc;
	int lastrtt;		// Measured RTT of last round-trip
	float lastpps;		// Measured PPS of last round-trip
	quint32 basewnd;
	float basertt, basepps, basepwr;

	// TCP Vegas-like congestion control
	float cwndmax;

	// Retransmit state
	Timer rtxtimer;		// Retransmit timer
	LinkStatus linkstat;	// Current link status

	// Receive state
	quint64 rxseq;		// Highest sequence number received so far
	quint32 rxmask;		// Mask of packets received so far

	// Receive-side ACK state
	quint64 rxackseq;	// Highest sequence number acknowledged so far
	//quint32 rxackmask;	// Mask of packets received & acknowledged
	quint8 rxackct;		// # contiguous packets received before rxackseq
	quint8 rxunacked;	// # contiguous packets not yet ACKed
	bool delayack;		// Enable delayed acknowledgments
	Timer acktimer;		// Delayed ACK timer

	// Statistics gathering
	float cumrtt;		// Cumulative measured RTT in milliseconds
	float cumrttvar;	// Cumulative variation in RTT
	float cumpps;		// Cumulative measured packets per second
	float cumppsvar;	// Cumulative variation in PPS
	float cumpwr;		// Cumulative measured network power (pps/rtt)
	float cumbps;		// Cumulative measured bytes per second
	float cumloss;		// Cumulative measured packet loss ratio
	Timer statstimer;


	// Transmit a packet across the flow.
	// Caller must leave hdrlen bytes at the beginning for the header.
	// The packet is armored in-place in the provided QByteArray.
	// It is the caller's responsibility to transmit
	// only when flow control says it's OK (mayTransmit())
	// or upon getting a readyTransmit() signal.
	// Provides in 'pktseq' the transmit sequence number
	// that was assigned to the packet.
	// Returns true if the transmit was successful,
	// or false if it failed (e.g., due to lack of buffer space);
	// a sequence number is assigned even on failure however.
	bool flowTransmit(QByteArray &pkt, quint64 &pktseq);


	// Check congestion control state and return the number of new packets,
	// if any, that flow control says we may transmit now.
	virtual int mayTransmit();

	// Compute current number of transmitted but un-acknowledged packets.
	// This count may include raw ACK packets,
	// for which we expect no acknowledgments
	// unless they happen to be piggybacked on data coming back.
	inline qint64 unackedPackets()
		{ return txseq - txackseq; }

	// Compute the time elapsed since the mark in microseconds.
	qint64 markElapsed();

public:
	// May be called by upper-level protocols during receive
	// to indicate that the packet has been received and processed,
	// so that subsequently transmitted packets include this ack info.
	// if 'sendack' is true, make sure an acknowledgment gets sent soon:
	// in the next transmitted packet, or in an ack packet if needed.
	void acknowledge(quint16 pktseq, bool sendack);

	inline bool deleyedAcks() const { return delayack; }
	inline void setDelayedAcks(bool enabled) { delayack = enabled; }

	void ccReset();
	inline CCMode ccMode() const { return ccmode; }
	inline void setCCMode(CCMode mode) { ccmode = mode; ccReset(); }

	// for CC_FIXED: fixed congestion window for reserved-bandwidth links
	inline void setCCWindow(int cwnd) { this->cwnd = cwnd; }

	// Congestion information accessors for flow monitoring purposes
	inline int txCongestionWindow() { return cwnd; }
	inline int txBytesInFlight() { return txfltsize; }
	inline int txPacketsInFlight() { return txfltcnt; }

signals:
	// Indicates when this flow observes a change in link status.
	void linkStatusChanged(LinkStatus newstatus);

protected:
	// Main method for upper-layer subclass to receive a packet on a flow.
	// Should return true if the packet was processed and should be acked,
	// or false to silently pretend we never received the packet.
	virtual bool flowReceive(qint64 pktseq, QByteArray &pkt) = 0;

	// Create and transmit a packet for acknowledgment purposes only.
	// Upper layer may override this if ack packets should contain
	// more than an just an empty flow payload.
	virtual bool transmitAck(QByteArray &pkt,
				quint64 ackseq, unsigned ackct);

	virtual void acked(quint64 txseq, int npackets, quint64 rxackseq);
	virtual void missed(quint64 txseq, int npackets);
	virtual void expire(quint64 txseq, int npackets);


private:
	// Called by Socket to dispatch a received packet to this flow.
	virtual void receive(QByteArray &msg, const SocketEndpoint &src);

	// Internal transmit methods.
	bool tx(QByteArray &pkt, quint32 packseq, quint64 &pktseq, bool isdata);
	inline bool txack(quint64 ackseq, unsigned ackct)
		{ QByteArray pkt; return transmitAck(pkt, ackseq, ackct); }
	inline void flushack()
		{ if (rxunacked) { rxunacked = 0; txack(rxackseq, rxackct); }
		  acktimer.stop(); }

	inline void rtxstart()
		{ rtxtimer.start((int)(cumrtt * 2.0)); }

	// Repeat stall indications but not other link status changes.
	// XXX hack - maybe "stall severity" or "stall time"
	// should be part of status?
	// Or perhaps status should be (up, stalltime)?
	inline void setLinkStatus(LinkStatus newstat)
		{ if (linkstat != newstat || newstat == LinkStalled) {
			linkStatusChanged(linkstat = newstat); } }

	// Congestion control
	void ccMissed(quint64 pktseq);


private slots:
	void rtxTimeout(bool failed);	// Retransmission timeout
	void ackTimeout();	// Delayed ACK timeout
	void statsTimeout();
};



// XX break this stuff into separate module

// Simple 32-bit keyed checksum protection with no encryption,
// to defend only against off-the-path attackers
// who can inject forged packets but not monitor the flow.
class ChecksumArmor : public FlowArmor
{
	const uint32_t txkey, rxkey;	// flow authentication parameters
	const QByteArray armorid;	// for key protocol duplicate detection

public:
	ChecksumArmor(uint32_t txkey, uint32_t rxkey,
			const QByteArray &armorid = QByteArray());

	virtual QByteArray txenc(qint64 pktseq, const QByteArray &pkt);
	virtual bool rxdec(qint64 pktseq, QByteArray &pkt);

	inline QByteArray id() { return armorid; }
};


class AESArmor : public FlowArmor
{
	const AES txaes, rxaes;
	const HMAC txmac, rxmac;

	union ivec {
		quint8 b[AES_BLOCK_SIZE];
		quint32 l[4];
	} ivec;

public:
	AESArmor(const QByteArray &txenckey, const QByteArray &txmackey,
		const QByteArray &rxenckey, const QByteArray &rxmackey);

	virtual QByteArray txenc(qint64 pktseq, const QByteArray &pkt);
	virtual bool rxdec(qint64 pktseq, QByteArray &pkt);
};


} // namespace SST

#endif	// SST_FLOW_H
