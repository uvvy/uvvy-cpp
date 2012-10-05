
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/rand.h>
#include <openssl/aes.h>

#include "hash.h"
#include "hub.h"
#include "peer.h"
#include "byteorder.h"


#define MAXDATA		1152		// Max size of user data in datagram

#define HDRLEN		16		// Size of flow header
#define MACLEN		16		// Size of MAC check in flow packets

#define RTT_INIT	(5000000)	// Initial RTT estimate: 1/2 second
#define RTT_MAX		(10*1000000)	// Max round-trip time: ten seconds

#define CWND_MIN	2		// Min congestion window (packets/RTT)
#define CWND_MAX	(1<<20)		// Max congestion window (packets/RTT)

#define ACKDELAY	(10*1000)	// 10 milliseconds
#define ACKPACKETS	2		// Max outstanding packets to be ACKed



struct FlowPacket
{
	uint32_t txseq;			// Transmit packet sequence number
	uint32_t ackseq;		// Acknowledgment sequence number
	uint32_t session;		// Session (24), AckCt (3), Window (5)
	uint32_t event;			// Event (24), Flags (4), Type (4)
	uint8_t data[MAXDATA+MACLEN];
};

// Subfields in FlowPacket.session
#define FPS_SESSBITS	24		// Bits 31-8: session pointer
#define FPS_SESSSHIFT	8
#define FPS_SESSMASK	((1<<FPS_SESSBITS)-1)
#define FPS_ACKCTBITS	3		// Bits 7-5: ACK packet count
#define FPS_ACKCTSHIFT	5
#define FPS_ACKCTMASK	((1<<FPS_ACKCTBITS)-1)
#define FPS_ACKCTMAX	FPS_ACKCTMASK
#define FPS_WINDOWBITS	5		// Bitts 4-0: log2-scale receive window
#define FPS_WINDOWSHIFT	0
#define FPS_WINDOWMASK	((1<<FPS_WINDOWBITS)-1)

// Subfields in FlowPacket.event
#define FPE_EVENTBITS	24		// Bits 31-8: event counter in session
#define FPE_EVENTSHIFT	8
#define FPE_EVENTMASK	((1<<FPE_EVENTBITS)-1)
#define FPE_ORIGIN	0x00000020	// Flag: session originated by sender
#define FPE_INSESSION	0x00000010	// Flag: packet is part of a session
#define FPE_TYPEBITS	4		// Bits 3-0: packet type
#define FPE_TYPESHIFT	0
#define FPE_TYPEMASK	((1<<FPE_TYPEBITS)-1)

// Packet types
#define FPT_ACK		0x0		// Raw ACK, no useful data
#define FPT_REFSET	0x1		// Set new reference point
#define FPT_REFCLR	0x2		// Clear old reference point
#define FPT_INIT	0x8		// (Sub)session-initiation packet
#define FPT_DATA	0x9		// Normal application datagram
#define FPT_DATAREF	0xa		// Normal data w/ new reference point
#define FPT_CLOSE	0xb		// Session close request

#define FPT_ISEVENT	0x8		// Bit flag: packet is an event



struct FlowSession;

// Packet buffer holding packets queued for transmit or receive
struct FlowBuf
{
	FlowBuf *next;		// Chain on various singly-linked lists
	FlowSession *sess;	// Session this packet belongs to
	size_t datasize;	// Size of user data in packet

	union {
		struct {	// State only used for packet transmission
			uint64_t evt[2];	// Event number of packet
			uint8_t	type;		// Packet type
		} tx;
		struct {	// State only used for packet reception
		} rx;
	};

	FlowPacket pkt;		// Packet buffer
};

// Public port listener
struct FlowPortEnt : public HashEnt
{
};

// An active reference point within a session
struct FlowRefEnt : public HashEnt
{
	FlowSession *sess;	// Session this reference point is part of
	uint64_t seq;		// TX/RX seqno of this refpoint in current flow
	uint64_t txevt[2];	// 128-bit TX event counter at this refpoint
	uint64_t rxevt[2];	// 128-bit RX event counter at this refpoint
};

struct FlowSession : public HashEnt	// Entry in Session ID hash table
{
	unsigned reliable : 1;	// Reliable transmission using event numbers

	FlowRefEnt txref;	// Entry in transmit reference point table
	FlowRefEnt rxref;	// Entry in receive reference point table

	// Transmit state for reliable delivery
	uint64_t txnxtevt[2];	// Next event number to assign
	uint64_t txackevt[2];	// Highest event number fully retired
	FlowBuf *txacked;	// Queue of ACKed but not yet retired events

	// Receive state
	uint8_t rxwin;		// Log2-scale receive window

	// Function for flow module to call to notify upper-level client
	// when more data can be accepted to transmit on this session.
	// Function can optionally return a FlowBuf to transmit immediately,
	// in which case flow module calls txpull repeatedly
	// as long as more packets are available and can still be accepted.
	FlowBuf *(*txpull)(void *data);

	void *data;


	FlowSession(int reliable);
};

struct FlowPeer
{
	Peer *peer;		// Back pointer to the main Peer struct

	// Transmit state
	uint64_t txseq;		// Next sequence number to transmit
	uint64_t txdatseq;	// Seqno of last real data packet transmitted
	uint64_t txackseq;	// Highest transmit sequence number ACK'd
	uint64_t txackmask;	// Mask of packets transmitted and ACK'd
	uint64_t markseq;	// Transmit sequence number of "marked" packet
	uint64_t marktime;	// Time at which marked packet was sent
	uint32_t markacks;	// Number of ACK'd packets since last mark
	uint32_t cwnd;		// Congestion window
	uint32_t ssthresh;	// Slow start threshold
	float rtt;		// Cumulative measured RTT in microseconds
	float rate;		// Measured transfer rate in packets per RTT
	AES_KEY txkey;		// Transmit encryption key

	// Transmit queue
	FlowBuf **txqhead;	// Next ptr in most recently queued packet
	FlowBuf *txqtail;	// Oldest queued packet (next to transmit)

	// Retransmit queue for reliable delivery,
	// always kept in order of txseq (transmit order).
	// Packets get deleted when ACKed,
	// and are moved to the head on retransmission.
	FlowBuf **rtxqhead;	// Next ptr in most recent outstanding packet
	FlowBuf *rtxqtail;	// Oldest outstanding transmitted packet

	Timer rtxtimer;		// Retransmit timer
	uint32_t rtxdelay;	// Retransmit delay (for exponential backoff)

	// Receive state
	uint64_t rxseq;		// Last sequence number received
	uint64_t rxmask;	// Mask of packets recently received
	uint8_t rxunacked;	// # contiguous packets not yet ACKed
	uint8_t rxackct;	// # contiguous packets received before rxseq
	Timer acktimer;		// Delayed ACK timer
	AES_KEY rxkey;		// Receive encryption key

	// Session lookup hash tables
	HashTab sesstab;	// Hash on session ID
	HashTab txreftab;	// Hash on transmit reference points
	HashTab rxreftab;	// Hash on receive reference points
	FlowSession rootsess;	// The root-level "anonymous" session


	FlowPeer(Peer *p);
	~FlowPeer();
	void cryptmsg(uint64_t seq, FlowPacket *dst, FlowPacket *src,
			size_t datasize, AES_KEY *key);
	int txpkt(FlowPacket *pkt, size_t datasize);
	int txsend();		// Send at least one packet from tx queue
	void txcheck();		// Send packets according to congestion control
	int txqueue(FlowSession *sess, unsigned type,
			const void *data, size_t datasize);
	void txack(uint64_t seq, uint8_t ackct);
	void flushack();
	void transmit(const void *msg, size_t size);
	void receive(FlowBuf *buf, size_t size);
};

#define TXACKMASKBITS		64	// Size of FlowPeer.txackmask in bits
#define RXMASKBITS		64	// Size of FlowPeer.rxmask in bits


int flowport = 5476;
int flowsock = -1;
int aggressive = 0;

static HashTab porttab;

static uint8_t dummykey[16];


static void rtxcallback(void *data);
static void ackcallback(void *data);

static inline FlowBuf *getbuf()
{
	return (FlowBuf*)malloc(sizeof(FlowBuf));
}

static inline void freebuf(FlowBuf *b)
{
	free(b);
}

FlowSession::FlowSession(int reliable)
{
	this->reliable = reliable;

	this->txnxtevt[0] = this->txnxtevt[1] = 0;
	this->txackevt[0] = this->txackevt[1] = 0;

	this->rxwin = 0;
}

FlowPeer::FlowPeer(Peer *peer)
:	rtxtimer(rtxcallback, this),
	acktimer(ackcallback, this),
	rootsess(0)
{
	this->peer = peer;
	peer->flowpeer = this;

	this->txseq = 1;
	this->txdatseq = 0;
	this->txackseq = 0;
	this->txackmask = 1;	// Ficticious packet 0 already "received"
	this->markseq = 1;
	this->cwnd = CWND_MIN;
	this->ssthresh = CWND_MAX;
	this->rtt = RTT_INIT;
	this->rate = 1;
	AES_set_encrypt_key(dummykey, sizeof(dummykey)*8, &txkey);

	this->txqhead = &this->txqtail;
	this->txqtail = NULL;

	this->rtxqhead = &this->rtxqtail;
	this->rtxqtail = NULL;
	this->rtxdelay = RTT_INIT;

	this->rxseq = 0;
	this->rxmask = 0;
	this->rxackct = 0;
	this->rxunacked = 0;
	AES_set_encrypt_key(dummykey, sizeof(dummykey)*8, &rxkey);
}

FlowPeer::~FlowPeer()
{
	assert(peer != NULL);
	assert(peer->flowpeer == this);

	peer->flowpeer = NULL;
	peer = NULL;
}

// Encrypt/decrypt a packet from 'src' to 'dst'.
// 'src' and 'dst' may be the same.
// 'size' includes the header and data but not MAC.
void FlowPeer::cryptmsg(uint64_t seq, FlowPacket *dst, FlowPacket *src,
			size_t datasize, AES_KEY *key)
{
	union {
		uint8_t b[16];
		uint32_t w[4];
		uint64_t l[2];
	} cblk, eblk;
	cblk.l[0] = htob64(seq);
	cblk.w[2] = htob32(0x55495063);		// 'UIPc'

	// Encrypt all but the txseq field of the flow packet header
	cblk.w[3] = htob32(0);
	AES_encrypt(cblk.b, eblk.b, key);
	dst->txseq = src->txseq;
	dst->ackseq = src->ackseq ^ eblk.w[1];
	dst->session = src->session ^ eblk.w[2];
	dst->event = src->event ^ eblk.w[3];

	// Encrypt complete data blocks from 'datasrc' into 'pkt'
	const uint64_t *s = (const uint64_t*)src->data;
	uint64_t *d = (uint64_t*)dst->data;
	size_t blk;
	size_t blks = (HDRLEN + datasize) >> 4;
//printf("crypt blks %d\n", blks);
	for (blk = 1; blk < blks; blk++) {
		cblk.w[3] = htob32(blk);
		AES_encrypt(cblk.b, eblk.b, key);
//printf("crypt seq %llu ofs %d: %016llx %016llx -> %016llx %016llx\n",
//	seq, blk, s[0], s[1], s[0] ^ eblk.l[0], s[1] ^ eblk.l[1]);
		*d++ = *s++ ^ eblk.l[0];
		*d++ = *s++ ^ eblk.l[1];
	}

	// Encrypt any final partial block
	size_t remain = datasize & 15;
	if (remain != 0) {
		cblk.w[3] = htob32(blk);
		AES_encrypt(cblk.b, eblk.b, key);

		size_t i;
		const uint8_t *sb = (const uint8_t*)s;
		uint8_t *db = (uint8_t*)d;
		for (i = 0; i < remain; i++)
			db[i] = sb[i] ^ eblk.b[i];
	}
}

// Low-level transmit routine:
// encrypt, authenticate, and transmit a packet
// whose cleartext header and data are already fully set up.
// Returns 0 on success, -1 on error (e.g., no output buffer space for packet)
int FlowPeer::txpkt(FlowPacket *pkt, size_t datasize)
{
	// Don't allow txseq counter to wrap (XX re-key before it does)
	if (txseq == (uint64_t)0-1) {
		warn("txseq counter overflow!");
		assert(0);	// XX
	}

	// Encrypt the message into a temporary packet buffer
	FlowPacket cpkt;
	cryptmsg(txseq, &cpkt, pkt, datasize, &txkey);

	// XXX authenticate

	// Bump transmit sequence number,
	// and timestamp if this packet is marked for RTT measurement
	// This is the "Point of no return" -
	// a failure after this still consumes sequence number space.
	assert(btoh32(pkt->txseq) == (uint32_t)txseq);
	if (txseq == markseq) {
		marktime = getdate();
		markacks = 0;
	}
	txseq++;

	// Ship it out
	if (sendto(flowsock, &cpkt, HDRLEN + datasize + MACLEN, 0,
			(const struct sockaddr*)&peer->sin,
			sizeof(peer->sin)) < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return -1;
		fatal("error sending on UDP socket to host %s: %s",
			peer->hostname, strerror(errno));
	}

	return 0;
}

// Transmit up to one packet waiting on the transmit queue.
// Returns 1 if a packet was waiting and sent successfully,
// returns 0 if the transmit queue is empty,
// and returns -1 if an error occurred during packet transmission
// (in which case the transmit queue is left untouched).
//
// XX there should be one transmit queue per session,
// with some appropriate scheduling algorithm
int FlowPeer::txsend()
{
	FlowBuf *b = txqtail;
	if (b == NULL)
		return 0;	// Nothing to send

	// Figure out how to indicate the session pointer
	FlowSession *sess = b->sess;
	uint32_t sessref = 0;
	uint32_t evtofs = 0;
	if (sess != &rootsess) {
		assert(0);	// XXX

		// Set a new reference point for this session
		// when sessref reaches half its maximum value.
		if (sessref >= FPS_SESSMASK/2) {
			// XXX
		}

		if (sess->reliable) {
			// Determine the event offset
		}
	}
	assert((sessref & ~FPS_SESSMASK) == 0);

	// Build the packet header
	unsigned type = b->tx.type;
	unsigned rxwin = sess->rxwin;
	assert(rxackct <= FPS_ACKCTMASK);
	assert(rxwin <= FPS_WINDOWMASK);
	assert(type <= FPE_TYPEMASK);
	b->pkt.txseq = htob32(txseq);
	b->pkt.ackseq = htob32(rxseq);
	b->pkt.session = htob32((sessref << FPS_SESSSHIFT) |
			(rxackct << FPS_ACKCTSHIFT) |
			(rxwin << FPS_WINDOWSHIFT));
	b->pkt.event = htob32((evtofs << FPE_EVENTSHIFT) |
			(type << FPE_TYPESHIFT));

	debug(2, "Flow: send seq %llu ack %llu(%d) type %d size %d",
		txseq, rxseq, rxackct, type, b->datasize);

	// Record the fact that this is "real data" for which we want an ACK.
	txdatseq = txseq;

	// Try to transmit it.
	if (txpkt(&b->pkt, b->datasize) < 0) {
		// Lower-level protocol's output queue is full -
		// just leave the packet on our transmit queue and return.
		debug(1, "Flow: can't send - UDP output buffer full?");
		return -1;
	}

	// Remove the packet from our transmit queue.
	txqtail = b->next;
	if (txqtail == NULL) {
		assert(txqhead == &b->next);
		txqhead = &txqtail;
	}

	if (sess->reliable) {

		// Save it on the session's retransmission queue (XX session's!)
		b->next = NULL;
		*rtxqhead = b;
		rtxqhead = &b->next;

	} else {

		// Unreliable packet - just free the buffer.
		freebuf(b);
	}

	// Start the retransmission timer, if it's not already running
	if (!rtxtimer.armed())
		rtxtimer.setafter(rtxdelay);

	return 1;
}

// Check the transmit queue and congestion control state,
// and send new packet(s) if appropriate.
void FlowPeer::txcheck()
{
	while (1) {

		// Check the congestion window.
		// Note that txackseq might be greater than txdatseq
		// if we have sent ACKs since our last real data transmission
		// and the other side has ACKed some of those ACKs.
		if ((txdatseq > txackseq) && ((txdatseq - txackseq) >= cwnd))
			return;		// Congestion window full

		// OK, congestion control says we can send a packet.
		if (txsend() < 1)
			return;
	}
}

int FlowPeer::txqueue(FlowSession *sess, unsigned type,
			const void *data, size_t datasize)
{
	assert(type <= FPE_TYPEMASK);
	assert(datasize <= MAXDATA);

	// Allocate a packet buffer
	FlowBuf *b = getbuf();
	if (b == NULL)
		return -1;

	// Initialize it with caller's parameters and data
	b->next = NULL;
	b->sess = sess;
	b->datasize = datasize;
	b->tx.type = type;
	memcpy(b->pkt.data, data, datasize);

	// Assign event numbers to packets in reliable sessions
	if (sess->reliable) {
		b->tx.evt[0] = sess->txnxtevt[0];
		b->tx.evt[1] = sess->txnxtevt[1];
		if (++sess->txnxtevt[1] == 0)
			++sess->txnxtevt[0];
	}

	// Queue the packet for transmission
	assert(*txqhead == NULL);
	*txqhead = b;
	txqhead = &b->next;

	// Go ahead and send it if possible
	txcheck();

	return 0;
}

static void rtxcallback(void *cbdata)
{
	FlowPeer *p = (FlowPeer*)cbdata;

	// Increase retransmit delay by a factor for exponential backoff
	p->rtxdelay = min((p->rtxdelay) * 3 / 2, RTT_MAX);

	// Force at least one new packet transmission regardless of cwnd.
	// This might not actually send a packet
	// if there's nothing on our transmit queue -
	// i.e., if no reliable sessions have outstanding data.
	// In that case, rtxtimer stays disarmed until the next transmit.
	p->txsend();
}

// Send a standalone ACK packet
void FlowPeer::txack(uint64_t ackseq, uint8_t ackct)
{
	assert(ackct <= FPS_ACKCTMASK);
	assert(rootsess.rxwin <= FPS_WINDOWMASK);

	debug(2, "Flow: send seq %llu ack %llu(%d) type ACK",
		txseq, ackseq, ackct);

	FlowPacket pkt;
	pkt.txseq = htob32(txseq);
	pkt.ackseq = htob32(ackseq);
	pkt.session = htob32((ackct << FPS_ACKCTSHIFT) |
				(rootsess.rxwin << FPS_WINDOWSHIFT));
	pkt.event = htob32(FPT_ACK << FPE_TYPESHIFT);
	(void)txpkt(&pkt, 0);	// Ignore errors
}

void FlowPeer::flushack()
{
	if (acktimer.armed()) {
		acktimer.disarm();
		rxunacked = 0;
		txack(rxseq, rxackct);
	}
}

static void ackcallback(void *cbdata)
{
	FlowPeer *p = (FlowPeer*)cbdata;
	p->rxunacked = 0;
	p->txack(p->rxseq, p->rxackct);
}

void FlowPeer::receive(FlowBuf *b, size_t size)
{
	// Validate the packet length
	if (size < HDRLEN+MACLEN) {
		drop:
		debug(1, "Flow: dropped invalid incoming packet");
		freebuf(b);
		return;
	}
	size_t datasize = size - (HDRLEN+MACLEN);
	b->datasize = datasize;

	// Determine the full 64-bit packet sequence number
	int32_t seqdiff = (int32_t)(btoh32(b->pkt.txseq) - (uint32_t)rxseq);
	uint64_t pktseq = rxseq + seqdiff;

	// Immediately drop too-old or already-received packets
	assert(RXMASKBITS == sizeof(rxmask)*8);
	if (seqdiff > 0) {
		if (pktseq < rxseq)
			goto drop;		// 64-bit wraparound!!
	} else if (seqdiff <= -RXMASKBITS) {
		goto drop;
	} else if (seqdiff <= 0) {
		if (rxmask & (1 << -seqdiff))
			goto drop;
	}

	// Authenticate the packet
	// XXX

	// Decrypt the packet
	cryptmsg(pktseq, &b->pkt, &b->pkt, datasize, &rxkey);

	// Get session and event words from packet header, in host byte order
	uint32_t pktsession = btoh32(b->pkt.session);
	uint32_t pktevent = btoh32(b->pkt.event);
	unsigned type = (pktevent >> FPE_TYPESHIFT) & FPE_TYPEMASK;

	// Update our receive state for this packet
	if (seqdiff == 1) {

		// Received packet is in-order and contiguous.
		// Roll rxseq and rxmask forward appropriately.
		rxseq = pktseq;
		rxmask = (rxmask << 1) + 1;
		rxackct++;
		if (rxackct > FPS_ACKCTMAX)
			rxackct = FPS_ACKCTMAX;

		// Delay our ACK for up to ACKPACKETS received non-ACK packets
		if (type != FPT_ACK) {
			rxunacked++;
			if (rxunacked < ACKPACKETS) {
				if (!acktimer.armed())
					acktimer.setafter(ACKDELAY);
			} else {
				rxunacked = 0;
				txack(rxseq, rxackct);
			}
		}

	} else if (seqdiff > 1) {

		// Received packet is in-order but discontiguous.
		// One or more packets probably were lost.
		// Flush any delayed ACK immediately.
		flushack();

		// Roll rxseq and rxmask forward appropriately.
		rxseq = pktseq;
		if (seqdiff < RXMASKBITS)
			rxmask = (rxmask << seqdiff) + 1;
		else
			rxmask = 1;	// bit 0 = packet just received

		// Reset the contiguous packet counter
		rxackct = 0;	// (0 means just 1 contiguous packet received)

		// ACK this discontiguous packet immediately
		// so that the sender is informed of lost packets ASAP.
		if (type != FPT_ACK)
			txack(rxseq, 0);

	} else {
		assert(pktseq < rxseq);

		// Out-of-order packet received.
		// Flush any delayed ACK immediately.
		flushack();

		// Set appropriate bit in rxmask for replay protection
		assert(seqdiff < 0 && seqdiff > -RXMASKBITS);
		rxmask |= (1 << -seqdiff);

		// ACK this out-of-order packet immediately.
		if (type != FPT_ACK)
			txack(pktseq, 0);
	}

	// Update our transmit state with the ack info in this packet
	int32_t ackdiff = (int32_t)(btoh32(b->pkt.ackseq) - (uint32_t)txackseq);
	uint64_t ackseq = txackseq + ackdiff;
	unsigned ackct = (pktsession >> FPS_ACKCTSHIFT) & FPS_ACKCTMASK;
	debug(2, "Flow: recv seq %llu ack %llu(%d) type %d size %d",
		pktseq, ackseq, ackct, type, datasize);
	if (ackseq >= txseq) {
		warn("ACK received for packet not yet transmitted!");
		goto drop;
	}
	unsigned newpackets = 0;
	if (ackdiff > 0) {

		// Received acknowledgment for one or more new packets.
		// Roll forward txackseq and txackmask.
		txackseq = ackseq;
		if (ackdiff < TXACKMASKBITS)
			txackmask <<= ackdiff;
		else
			txackmask = 0;

		// Record the last (ackct+1) packets in txackmask as received.
		txackmask |= (1 << (ackct+1))-1;

		// Determine the number of actual newly-acknowledged packets.
		newpackets = min(ackct+1, (unsigned)ackdiff);

		// Detect packet loss/delay
		if ((unsigned)ackdiff > newpackets) {

			// Packet loss detected - perform congestion control
			ssthresh = max((txseq - txackseq) / 2, CWND_MIN);
			cwnd = ssthresh;
		}

		// Reset the retransmission timer, since we've made progress.
		// Only re-arm it if there's still outstanding unACKed data.
		rtxdelay = (uint32_t)rtt * 3 / 2;
		if (rtxtimer.armed())
			rtxtimer.disarm();
		if (txdatseq > txackseq)
			rtxtimer.setafter(rtxdelay);

	} else {

		// Received acknowledgment for straggling out-of-order packets
		// (or an out-of-order acknowledgment for in-order packets).
		// Set the appropriate bits in our txackmask,
		// and count newly acknowledged packets within our window.
		for (unsigned i = 0; i <= ackct; i++) {
			unsigned d = -ackdiff + i;
			if (d >= TXACKMASKBITS)
				break;
			if (txackmask & (1 << d))
				continue;	// already ACKed
			txackmask |= (1 << d);
			newpackets++;
		}
	}

	// Count the total number of acknowledged packets since the last mark.
	markacks += newpackets;

	// During slow start, increment cwnd for each newly-ACKed packet.
	if (cwnd <= ssthresh)
		cwnd += newpackets;

	// When ackseq passes markseq, we've observed a round-trip,
	// so update our round-trip statistics.
	if (ackseq >= markseq) {

		// 'delay' is the total round-trip delay before
		// we received the ACK for the last marked packet.
		// Fold this into 'rtt' to determine avg round-trip time.
		uipdate delay = getdate() - marktime;
		if (delay < 0) delay = 0;
		if (delay > RTT_MAX) delay = RTT_MAX;
		rtt = ((rtt * 7.0) + delay) / 8.0;

		// 'markacks' is the number of unique packets ACKed
		// by the receiver during the time since the last mark.
		// Fold this into 'rate' to determine avg packets per RTT.
		rate = ((rate * 7.0) + min(markacks, CWND_MAX)) / 8.0;

		// Reset markseq to be the next packet transmitted.
		// The new timestamp will be taken when that packet is sent.
		markseq = txseq;

		// During congestion avoidance,
		// increment cwnd once each RTT.
		cwnd++;
	}

	// Clamp cwnd against CWND_MAX,
	// and also don't let it exceed twice the actual
	// current number of outstanding packets in the network.
	// This can happen if our actual transmission rate
	// remains lower than the network's capacity for some period of time.
	cwnd = min(cwnd, CWND_MAX);
	cwnd = min(cwnd, (txseq - txackseq) * 2);

	// Look up the session this packet belongs to
	FlowSession *sess;
	FlowRefEnt *ref;
	if (pktevent & FPE_INSESSION) {

		// Packet directed at a session reference point - find it
		uint32_t seqofs = pktsession >> FPS_SESSSHIFT;
		if (pktevent & FPE_ORIGIN) {
			// Session initiated by sender (other guy)
			if (seqofs > pktseq) {
				warn("Invalid session pointer in packet");
				goto drop;
			}
			uint64_t rxseq = pktseq - seqofs;
			ref = (FlowRefEnt*)rxreftab.lookup(
						&rxseq, sizeof(rxseq));
		} else {
			// Session initiated by receiver (us)
			if (seqofs > ackseq) {
				warn("Invalid session pointer in packet");
				goto drop;
			}
			uint64_t txseq = ackseq - seqofs;
			ref = (FlowRefEnt*)txreftab.lookup(
						&txseq, sizeof(txseq));
		}
		if (ref == NULL) {
			warn("unknown session!");
			goto drop;	// XXX query
		}
		sess = ref->sess;

	} else {

		// Directed at a port - use the anonymous "root" session,
		// no reference point available (no reliable transmission)
		sess = &rootsess;
		ref = NULL;
	}

	switch (type) {
	case FPT_ACK:
		// Nothing more to do with it.
		break;

	case FPT_DATA:
		// XX hand it off to the receiver
		gossip_receive(peer, &b->pkt.data, datasize);
		break;

	default:
		debug(1, "Flow: unknown packet type %d", type);
		break;
	}
	freebuf(b);
}

void flow_init()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		fatal("can't create UDP socket: %s", strerror(errno));

	struct sockaddr_in mysin;
	mysin.sin_family = AF_INET;
	mysin.sin_addr.s_addr = htonl(INADDR_ANY);
	mysin.sin_port = htons(flowport);
	if (bind(sock, (const struct sockaddr*)&mysin, sizeof(mysin)) < 0)
		fatal("can't bind UDP socket to port %d: %s",
			flowport, strerror(errno));

	setnonblock(sock);
	watchfd(sock, 0, 1);
	flowsock = sock;
}

void flow_initpeer(Peer *p)
{
	p->flowpeer = new FlowPeer(p);
}

void flow_delpeer(Peer *p)
{
	assert(p->flowpeer != NULL);
	delete p->flowpeer;
}

void flow_check(fd_set *readfds, fd_set *writefds)
{
	if (FD_ISSET(flowsock, readfds)) {
		while (1) {
			// Grab a buffer to receive into
			FlowBuf *b = getbuf();
			FlowBuf dropbuf;
			if (b == NULL)
				b = &dropbuf;

			// Receive a datagram
			struct sockaddr_in sin;
			socklen_t sinlen = sizeof(sin);
			ssize_t sz = recvfrom(flowsock,
					&b->pkt, sizeof(b->pkt), 0,
					(struct sockaddr*)&sin, &sinlen);
			if (sz < 0) {
				if (errno == EAGAIN || errno == EINTR)
					break;
				fatal("error receiving from UDP socket: %s",
					strerror(errno));
			}
			assert(sinlen == sizeof(sin));
			assert(sin.sin_family == AF_INET);

			// Just drop it if we couldn't find a free buffer
			if (b == &dropbuf) {
				warn("Packet dropped due to lack of memory");
				continue;
			}

			// Dispatch the received packet
			for (Peer *p = peers; p != NULL; p = p->next) {
				if (p->sin.sin_addr.s_addr ==
						sin.sin_addr.s_addr &&
						p->sin.sin_port ==
						sin.sin_port) {
					p->flowpeer->receive(b, sz);
				}
			}
		}
	}
}

// XX transmit a packet on a peer's root session
void flow_transmit(Peer *p, const void *msg, size_t size)
{
	p->flowpeer->txqueue(&p->flowpeer->rootsess, FPT_DATA, msg, size);
}

