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

#include <math.h>
#include <netinet/in.h>

#include <QtDebug>

#include "tcp.h"
#include "util.h"
#include "host.h"

using namespace SST;


#define TCP_MAGIC	0x00544350	// 'TCP'

struct TcpHeader
{
	quint32	magic;		// in place of srcport, dstport
	//quint16 srcport;
	//quint16 dstport;

	qint32	seqno;
	qint32	ackno;
	quint16	flags;		// Flags and data offset
	quint16 window;
	quint16 checksum;
	quint16 urgent;
};

#define FIN	0x0001
#define SYN	0x0002
#define RST	0x0004
#define PSH	0x0008
#define ACK	0x0010
#define URG	0x0020

#define DataMsk	0xf000
#define DataShf	12


// Max number of SACK blocks to include in a TCP segment
#define NSACKS	4


////////// TcpFlow //////////

const int TcpFlow::smss;
const int TcpFlow::cwndMin;
const int TcpFlow::cwndMax;
const int TcpFlow::rttInit;
const int TcpFlow::rttMax;
const int TcpFlow::dupthresh;
const int TcpFlow::minHdr;
const int TcpFlow::maxHdr;
const int TcpFlow::defaultWindow;
const int TcpFlow::ackDelay;

TcpFlow::TcpFlow(Host *host, TcpStream *strm)
:	host(host), strm(strm),
	state(Closed), lisn(false), endread(false), endwrite(false),
	dupcnt(0),
	ssthresh(cwndMax), cwnd(cwndMin), cwndlim(false),
	rtxtimer(host), flightsize(0), fastrecov(false),
	rxwin(defaultWindow), rxavail(0), rxunacked(0),
	delayack(true), acktimer(host),
	cumrtt(rttInit), cumrttvar(0)
{
	// Choose random initial transmit sequence number
	QByteArray r = randBytes(4);
	memcpy(&txseq.v, r.data(), 4);
	txseq.v = 1;	// XXX
	txlim = txseq+1;	// Make sure receive window allows sending SYN
	txasn = ackseq = markseq = TcpSeq(-1);	// "uninitialized"

	// Send zero in the ACK field until we get synchronized.
	rxseq.v = 0;

	// Hook up the timers
	connect(&rtxtimer, SIGNAL(timeout(bool)),
		this, SLOT(rtxTimeout(bool)));
	connect(&acktimer, SIGNAL(timeout(bool)),
		this, SLOT(ackTimeout()));
}

TcpFlow::~TcpFlow()
{
	qDebug() << "~TcpFlow";
}

void TcpFlow::connectToHost(const QHostAddress &addr, quint16 port)
{
	Q_ASSERT(state == Closed);
	Q_ASSERT(remoteep.sock == NULL);

	// Create a new local socket for our end of the connection.
	remoteep.addr = addr;
	remoteep.port = port;
	remoteep.sock = host->newSocket(this);

	// Bind the socket to a UDP port on this host.
	bool success = remoteep.sock->bind();
	Q_ASSERT(success);

	// Bind our SocketFlow to this socket to receive packets.
	success = bind(remoteep.sock, remoteep, 0);
	Q_ASSERT(success);

	// Queue up a SYN packet.
	state = SynSent;
	TxSegment seg;
	seg.tsn = txseq;
	seg.buf.resize(maxHdr);
	seg.datalen = 0;
	seg.flags = SYN;
	txseq += 1;
	txq.enqueue(seg);
	tryTransmit();
}

void TcpFlow::listen(const SocketEndpoint &src)
{
	Q_ASSERT(state == Closed);

	// Save the remote (initiator's) endpoint and socket
	remoteep = src;

	// Bind our SocketFlow to this socket to receive packets.
	bool success = bind(remoteep.sock, remoteep, 0);
	Q_ASSERT(success);

	// Set it to the listening state.
	lisn = true;
	state = Listen;
}

// Main QIODevice entrypoint to write data to the stream.
qint64 TcpFlow::writeData(const char *data, qint64 totsize)
{
	Q_ASSERT(!endwrite);

	if (totsize == 0)
		return 0;
	Q_ASSERT(totsize > 0);

	qint64 actsize = 0;
	do {
		// Choose the size of this segment.
		int size = smss;
		quint8 flags = 0;
		if (totsize <= size) {
			flags = PSH;
			size = totsize;
		}
		//qDebug() << "Transmit segment at" << txasn.v
		//	<< "size" << size;

		// Set up the segment info block and packet payload,
		// but leave the header to be filled in on each retransmission.
		TxSegment seg;
		seg.tsn = txasn;
		seg.buf.resize(maxHdr + size);
		memcpy(seg.buf.data() + maxHdr, data, size);
		seg.datalen = size;
		seg.flags = flags;
		txasn += size;

		// Queue up the segment, and transmit if possible.
		txq.enqueue(seg);
		tryTransmit();

		// On to the next segment...
		data += size;
		totsize -= size;
		actsize += size;
	} while (totsize > 0);
	return actsize;
}

void TcpFlow::endWrite()
{
	if (endwrite)
		return;

	endwrite = true;

	// Queue up a FIN packet
	TxSegment seg;
	seg.tsn = txasn;
	seg.buf.resize(maxHdr);
	seg.datalen = 0;
	seg.flags = FIN;
	txasn += 1;
	txq.enqueue(seg);
	tryTransmit();
}

// Pull something off our transmit queue and send it,
// if our congestion and receive windows allow.
void TcpFlow::tryTransmit()
{
	while (true) {
		//qDebug() << "flightsize" << flightsize << "cwnd" << cwnd;
		if (flightsize > cwnd) {
			cwndlim = true;
			return;		// No space in congestion window.
		}
		if (!doTransmit())
			return;		// Nothing we can transmit.
	}
}

// Pull one packet off our transmit queue and send it.
bool TcpFlow::doTransmit()
{
	// Transmit new data packets if we can.
	if (txq.isEmpty())
		return false;		// Nothing to transmit.

	TxSegment &seg = txq.first();
	TcpSeq segmax = seg.tsn + seg.datalen;
	if (segmax > txlim)
		return false;		// Doesn't fit into receive window.

	// Attach a piggybacked ACK if appropriate (which it usually is).
	switch (state) {
	case Closed:
	case Listen:
		Q_ASSERT(0);	// shouldn't get here
	case SynSent:
		break;		// No piggybacked ACK
	default:
		seg.flags |= ACK;
		break;
	}

	// Transmit the segment.
	txSegment(seg, rxseq);

	// Record that we've transmitted up to this point
	if (txseq < seg.tsn+seg.datalen)
		txseq = seg.tsn+seg.datalen;

	// If we just sent a FIN, transition to the appropriate state
	if (seg.flags & FIN) {
		txseq += 1;
		switch (state) {
		case Estab:
			state = FinWait1;
			break;
		case CloseWait:
			state = LastAck;
			break;
		default:
			qFatal("Sending FIN in state %d!?", state);
		}
	}

	// Bump mark timestamp if this segment is marked for RTT measurement
	if (seg.tsn == markseq)
		marktime = host->currentTime();

	// Enqueue it onto rtxq and dequeue it from txq,
	// being careful to preserve original transmission order.
	// The flightsize tracks the un-SACKed bytes in rtxq.
	flightsize += seg.datalen;
	txInsert(rtxq, seg);
	txq.removeFirst();

	return true;
}

inline qint64 TcpFlow::markElapsed()
{
	return host->currentTime().since(marktime).usecs;
}

void TcpFlow::txInsert(QQueue<TxSegment> &q, const TxSegment &seg)
{
	int i = 0;
	while (i < q.size() && q[i].tsn < seg.tsn)
		i++;
	q.insert(i, seg);
}

// Transmit or retransmit a data segment.
void TcpFlow::txSegment(TxSegment &seg, TcpSeq ackno)
{
	if (0) qDebug() << "TX"
		<< (seg.flags & SYN ? "SYN" : "")
		<< (seg.flags & FIN ? "FIN" : "")
		<< (seg.flags & RST ? "RST" : "")
		<< (seg.flags & ACK ? "ACK" : "")
		<< (seg.flags & PSH ? "PSH" : "")
		<< "seq" << seg.tsn.v << "ack" << ackno.v
		<< "size" << seg.datalen << "wnd" << rxwin;

	int hdrofs;
	if (rxsack.isEmpty()) {
		// No SACK information to send - just the fixed TCP header.
		hdrofs = maxHdr - minHdr;
	} else {
		// Position TCP header based on number of SACK ranges to send,
		// and fill in the SACK header option.
		int nsacks = rxsack.size();
		int sackhdrofs = maxHdr - (4 + nsacks*8);
		quint32 *p = (quint32*)(seg.buf.data() + sackhdrofs);
		*p++ = htonl(0x01010500 + 2+nsacks*8);
		for (int i = 0; i < nsacks; i++) {
			*p++ = htonl(rxsack[i].first.v);
			*p++ = htonl(rxsack[i].second.v);
		}
		Q_ASSERT((char*)p == seg.buf.data() + maxHdr);
		hdrofs = sackhdrofs - minHdr;
	}
	int hdrlen = maxHdr - hdrofs;

	// Fill in the fixed part of the TCP header.
	Q_ASSERT(sizeof(TcpHeader) == minHdr);
	TcpHeader *h = (TcpHeader*)(seg.buf.data() + hdrofs);
	h->magic = htonl(TCP_MAGIC);
	h->seqno = htonl(seg.tsn.v);
	h->ackno = htonl(ackno.v);
	h->flags = htons(seg.flags | (hdrlen/4 << DataShf));
	h->window = htons(rxwin);
	h->checksum = 0;	// XXX
	h->urgent = 0;

	// If the retransmission timer is inactive, start it afresh.
	// If this was a retransmission, rtxTimeout() would have restarted it.
	if (txseq > ackseq && !rtxtimer.isActive()) {
		//qDebug() << "rtxstart at time"
		//	<< QDateTime::currentDateTime()
		//		.toString("h:mm:ss:zzz");
		rtxstart();
	}

	// Transmit!
	remoteep.sock->send(remoteep, (char*)h, seg.buf.size() - hdrofs);
}

// Transmit a control packet containing no data but specified header fields.
void TcpFlow::txControl(quint16 flags, TcpSeq seqno, TcpSeq ackno)
{
	// Prepare a segment containing no data
	TxSegment seg;
	seg.tsn = seqno;
	seg.buf.resize(maxHdr);
	seg.datalen = 0;
	seg.flags = flags;
	txSegment(seg, ackno);
}

void TcpFlow::endRead()
{
	endread = true;
}

void TcpFlow::receive(QByteArray &pkt, const SocketEndpoint &)
{
	TcpHeader *h = (TcpHeader*)pkt.data();
	if (ntohl(h->magic) != TCP_MAGIC)
		return qDebug("Received packet with bad TCP magic");
	quint16 flags = ntohs(h->flags);

	TcpSeq seqno, ackno;
	seqno.v = ntohl(h->seqno);
	ackno.v = ntohl(h->ackno);

	int hdrlen = (flags >> DataShf) * 4;
	int datalen = pkt.size() - hdrlen;
	if (hdrlen < 20 || datalen < 0)
		return qDebug("Received packet with bad header length");

	if (0) qDebug() << "RX"
		<< (flags & SYN ? "SYN" : "")
		<< (flags & FIN ? "FIN" : "")
		<< (flags & RST ? "RST" : "")
		<< (flags & ACK ? "ACK" : "")
		<< (flags & PSH ? "PSH" : "")
		<< "seq" << seqno.v << "ack" << ackno.v
		<< "size" << datalen << "wnd" << ntohs(h->window);

	// Process RST segments
	if (flags & RST) {
		// rfc793, page 36, "Reset Processing"
		// First validate sequence number.
		if (state == SynSent) {
			if (!(flags & ACK) || (ackno != txseq))
				return;		// unacceptable - drop
		} else {
			if (seqno < rxseq || seqno > rxseq+rxwin)
				return;		// unacceptable - drop
		}
		// Reset validated - process it.
		qDebug("Connection reset received in state %d", state);
		if (state == Listen)
			return;
		return goClosed();
	}

	// Validate the incoming segment's sequence number.
	// rfc793, page 35, "Reset Generation"
	switch (state) {

	case Closed:
		// Any segment except a reset (handled above) solicits a reset.
		// rfc793, page 35, case 1.
		if (flags & ACK)
			return txControl(RST, ackno, 0);
		else
			return txControl(RST|ACK, 0, seqno+datalen);

	// rfc793, page 35, case 2.
	case Listen:
		// Validate sequence number: rfc793, page 35, case 2.
		if (flags & ACK)
			return txControl(RST, ackno, 0);
		if (!(flags & SYN))
			return txControl(RST|ACK, 0, seqno+datalen);

		// Received valid SYN - record sender's ISN.
		rxseq = seqno+1;
		state = SynRcvd;
		return txControl(SYN|ACK, txseq-1, rxseq);

	case SynSent:
		// Validate sequence number: rfc793, page 35, case 2.
		if ((flags & ACK) && (ackno != txseq))
			return txControl(RST, ackno, 0);
		if (!(flags & SYN))
			return txControl(RST|ACK, 0, seqno+datalen);

		// Received valid SYN - record sender's ISN.
		rxseq = seqno+1;

		// If we didn't get an ACK, may be a simultaneous open -
		// go to SynRcvd state to wait for the ACK for our SYN.
		if (!(flags & ACK)) {
			state = SynRcvd;
			return txControl(SYN|ACK, txseq-1, rxseq);
		}

		// Received valid SYN/ACK - connection established.
		//qDebug() << "Connection established actively";
		txasn = txlim = ackseq = markseq = txseq;
		state = Estab;
		txControl(ACK, txseq, rxseq);
		strm->setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
		strm->connected();
		break;

	case SynRcvd:
		// Validate sequence number: rfc793, page 35, case 2.
		if ((flags & ACK) && (ackno != txseq))
			return txControl(RST, ackno, 0);
		if (!(flags & ACK))
			return;		// didn't get the desired ACK

		// Received valid ACK for our SYN - connection established.
		//qDebug() << "Connection established passively";
		txasn = txlim = ackseq = markseq = txseq;
		state = Estab;
		strm->setOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
		strm->connected();
		break;

	default:
		break;
	}

	if (flags & SYN)
		seqno += 1;	// account for but ignore duplicate SYN

	// Validate sequence number: rfc793, page 35, case 3.
	if (seqno+datalen < rxseq) {
		//qDebug() << "Unacceptable segment: obsolete";
		return txControl(ACK, txseq, rxseq);
	}
	if (seqno > rxseq+rxwin) {
		qDebug() << "Unacceptable segment: above receive window";
		return txControl(ACK, txseq, rxseq);
	}
	if ((flags & ACK) && (ackno > txseq)) {
		qDebug() << "Unacceptable segment: acknowledges unsent data";
		return txControl(ACK, txseq, rxseq);
	}

	// Process the data received in the segment.
	if (datalen == 0 && !(flags & FIN)) {
		// No data to process.
		//qDebug() << "Received empty ACK";

	} else if (seqno <= rxseq) {

		// This data will need an acknowledgment, soon if not now.
		++rxunacked;

		// Received some data in-order: process it immediately.
		bool wasempty = rxsegs.isEmpty();
		gotSegment(flags, seqno, hdrlen, datalen, pkt);

		// Process waiting out-of-order segments
		while (!rahead.isEmpty()) {
			RxSegment &seg = rahead.first();
			if (seg.rsn > rxseq)
				break;	// there's still a gap

			// Gap closed - process this stored segment.
			TcpHeader *h = (TcpHeader*)seg.buf.data();
			quint16 flags = ntohs(h->flags);
			qint32 seqno = ntohl(h->seqno);
			gotSegment(flags, seqno, seg.hdrlen, seg.datalen,
					seg.buf);
			rahead.removeFirst();
		}

		// Clear any relevant ranges in our SACK state.
		for (int i = rxsack.size()-1; i >= 0; i--) {
			if (rxsack[i].first > rxseq)
				continue;	// not there yet

			// Overlap: we must have processed the entire range.
			Q_ASSERT(rxsack[i].second <= rxseq);
			rxsack.removeAt(i);
		}

		if (wasempty && strm)
			strm->readyRead();

	} else {
		// Received some data out-of-order.
		// This data will need an acknowledgment, soon if not now.
		++rxunacked;
		//qDebug() << "Received data out-of-order at" << seqno.v;

		// Insert the out-of-order segment into the rahead list,
		// keeping the list sorted by sequence number.
		int i = 0;
		while (i < rahead.size() && rahead[i].rsn < seqno)
			i++;

		// Only save it if it appears to provide some data
		// that we don't already have.
		if (i == rahead.size() || seqno < rahead[i].rsn ||
			seqno+datalen > rahead[i].rsn+rahead[i].datalen) {

			RxSegment seg;
			seg.rsn = seqno;
			seg.buf = pkt;
			seg.hdrlen = hdrlen;
			seg.datalen = datalen;
			rahead.insert(i, seg);
		}

		// Add or merge the segment into our SACK state.
		TcpSeq sackmin = seqno;
		TcpSeq sackmax = seqno + datalen;
		for (i = rxsack.size()-1; i >=0; i--) {
			RxRange &sack = rxsack[i];
			if (sackmax < sack.first || sackmin > sack.second)
				continue;	// no overlap

			// This range overlaps - merge and remove it.
			if (sack.first < sackmin)
				sackmin = sack.first;
			if (sack.second > sackmax)
				sackmax = sack.second;
			rxsack.removeAt(i);
		}
		rxsack.prepend(RxRange(sackmin, sackmax));
		if (rxsack.size() > NSACKS)
			rxsack.removeLast();
	}

	// Parse the SACK header, and find the current SACK limit.
	TcpSeq sackseq = ackseq;
	bool progress = true;
	if (hdrlen > minHdr) {
		quint32 *p = (quint32*)(h+1);
		quint32 w = ntohl(*p++);
		Q_ASSERT((w & 0xffffff00) == 0x01010500);	// XXX
		int len = (w & 0xff) - 2;
		Q_ASSERT(len == hdrlen - minHdr - 4);		// XXX
		for (int i = 0; i < len/8; i++) {
			TcpSeq sackmin(ntohl(*p++));
			TcpSeq sackmax(ntohl(*p++));
			//qDebug() << "Got SACK range:" << sackmin.v
			//		<< "to" << sackmax.v;
			if (sackmax <= sackmin) {
				qDebug("Invalid SACK range!");
				continue;
			}
			if (sackmax > sackseq)
				sackseq = sackmax;

			// Just remove all SACKed segments from rtxq.
			// XXX This is not RFC compliant behavior,
			// since SACKs are suppost to be advisory only -
			// but we can assume they're definitive here
			// because we know the other host treats tham that way,
			// and deleting the buffers early doesn't change
			// other protocol behavior significantly.
			for (int i = rtxq.size()-1; i >= 0; i--) {
				TxSegment &seg = rtxq[i];
				if (seg.tsn+seg.datalen <= sackmin)
					continue;
				if (seg.tsn >= sackmax)
					continue;

				//qDebug() << "Got SACK for seg" << seg.tsn.v;
				flightsize -= seg.datalen;
				Q_ASSERT(flightsize >= 0);
				rtxq.removeAt(i);

				progress = true;
			}
		}
	}

	// Update our transmit state based on the acknowledgment information
	// in the received data segment.
	if (ackno > ackseq) {

		// This packet acknowledges new data.
		ackseq = ackno;
		dupcnt = 0;
		fastrecov = false;
		progress = true;

		// Clear out acknowledged segments from rtxq,
		// and adjust the flightsize total appropriately.
		while (!rtxq.isEmpty()) {
			TxSegment &seg = rtxq.first();
			if (seg.tsn+seg.datalen > ackno)
				break;	// nothing more we can remove yet

			flightsize -= seg.datalen;
			Q_ASSERT(flightsize >= 0);
			rtxq.removeFirst();
		}

		// Expand the congestion window for each ACK during slow start.
		if (cwndlim && cwnd < ssthresh) {
			//qDebug() << "Slow start: cwnd" << cwnd
			//	<< "ssthresh" << ssthresh;
			cwnd += smss;
		}

		// Handle an ACK of our FIN
		if (endwrite && ackseq == txasn) {
			//qDebug() << "Got ACK to our FIN";
			switch (state) {
			case FinWait1:
				state = FinWait2;
				break;
			case Closing:
				goTimeWait();
				break;
			case LastAck:
				goClosed();
				break;
			case FinWait2:
			case TimeWait:
				break;	// ignore redundant ACKs
			default:
				Q_ASSERT(0);	// shouldn't happen
			}
		}

	} else if (ackseq == txseq) {

		// No progress, but there wasn't supposed to be any...
		dupcnt = 0;
		fastrecov = false;

	} else if (sackseq > ackseq && ++dupcnt >= dupthresh && !fastrecov) {

		// Received too many duplicate acknowledgments -
		// go into fast retransmit and fast recovery.
		// We do NOT need to inflate cwnd artificially
		// as described in rfc2581, secion 3.2,
		// because that algorithm assumes that we have no other way
		// of accounting for out-of-order packets leaving the network.
		// In this case, we do, through the SACK mechanism,
		// which decreases flightsize as packets get sacked.

		//qDebug() << "Entering fast recovery";
		// XXX ssthresh = qMax(flightsize / 2, smss*2);
		ssthresh = qMax(cwnd / 2, smss*2);
		cwnd = ssthresh;

		// Move all packets in rtxq up to sackseq back to txq,
		// for retransmission.
		// These are the packets that have presumably left the network.
		while (!rtxq.isEmpty()) {
			TxSegment &seg = rtxq.first();
			if (seg.tsn >= sackseq)
				break;

			txInsert(txq, seg);
			flightsize -= seg.datalen;
			Q_ASSERT(flightsize >= 0);
			rtxq.removeFirst();
		}

		// We're now in fast recovery until ackno increases.
		fastrecov = true;
	}

	// Reset the retransmission timer if we've made effective progress.
	// Only re-arm it if there's still outstanding unACKed data.
	if (progress && state != TimeWait) {
		if (txseq > ackseq) {
			//qDebug() << "receive: rtxstart at time"
			//	<< QDateTime::currentDateTime()
			//		.toString("h:mm:ss:zzz");
			rtxstart();
		} else
			rtxtimer.stop();
	}

	// Update our transmit-side information about the receive window
	txlim = ackno + ntohs(h->window);

	// When ackseq passes markseq, we've observed a round-trip,
	// so update our round-trip statistics.
	if (ackseq > markseq) {

		// 'rtt' is the total round-trip delay in microseconds before
		// we receive an ACK for a packet at or beyond the mark.
		// Fold this into 'rtt' to determine avg round-trip time,
		// and restart the timer to measure the next round-trip.
		int rtt = markElapsed();
		rtt = qMax(1, qMin(rttMax, rtt));
		cumrtt = ((cumrtt * 7.0) + rtt) / 8.0;

		// Compute an RTT variance measure
		float rttvar = fabsf(rtt - cumrtt);
		cumrttvar = ((cumrttvar * 7.0) + rttvar) / 8.0;

		// Reset markseq to be the next new segment transmitted.
		// The new timestamp will be taken when that segment is sent.
		markseq = txseq;

		// During congestion avoidance,
		// increment cwnd by 1*SMSS per round-trip time,
		// but only on round-trips that were cwnd-limited.
		if (cwndlim) {
			cwnd += smss;
			//qDebug("cwnd %d ssthresh %d", cwnd, ssthresh);
		}

		//qDebug() << "Measured RTT" << rtt << "cum" << cumrtt
		//	<< "var" << cumrttvar;
	}

	// Always clamp cwnd against cwndMax.
	cwnd = qMin(cwnd, cwndMax);

	// Transmit more packets in response, if cwnd allows it.
	tryTransmit();

	// If we still owe the sender an ACK
	// even after tryTransmit() had a chance to send a piggybacked ack,
	// then either send an explicit ack now or delay it if appropriate.
	if (rxunacked) {
		if (delayack && rxunacked < 2) {
			// Start delayed ack timer.
			if (!acktimer.isActive())
				acktimer.start(ackDelay);
		} else {
			// Need a prompt acknowledgment now.
			flushAck();
		}
	}
}

void TcpFlow::gotSegment(quint16 flags, TcpSeq seqno,
			int hdrlen, int datalen, QByteArray &pkt)
{
	//qDebug() << "gotSegment at" << seqno.v << "size" << datalen;

	// Throw away any prefix of the segment
	// that duplicates data that we've already received.
	qint32 duplen = rxseq - seqno;
	if (duplen) {
		qDebug() << duplen << "of" << datalen << "bytes duplicated";
		Q_ASSERT(duplen > 0);
		hdrlen += duplen;
		datalen -= duplen;
	}

	// Enqueue the segment if it contains any useful data.
	if ((datalen > 0) || (flags & FIN)) {
		RxSegment seg;
		seg.buf = pkt;
		seg.hdrlen = hdrlen;
		seg.datalen = datalen;
		seg.flags = flags;
		rxsegs.enqueue(seg);
		rxavail += datalen;
		if (flags & FIN)
			rxavail++;
		rxseq += datalen;
	}

	// Stream close processing.
	if (!(flags & FIN))
		return;
	switch (state) {
	case Estab:
		rxseq += 1;	// account for FIN in sequence space
		state = CloseWait;
		break;

	case FinWait1:
		rxseq += 1;	// account for FIN in sequence space
		state = Closing;
		break;

	case FinWait2:
		rxseq += 1;	// account for FIN in sequence space
		goTimeWait();
		break;

	case Closing:
	case CloseWait:
	case LastAck:
	case TimeWait:
		break;		// Acknowledge but ignore duplicate FIN

	default:
		Q_ASSERT(0);	// Should not happen
	}
	//qDebug() << "gotSegment: FIN, rxseq" << rxseq.v;
}

qint64 TcpFlow::readData(char *data, qint64 maxsize)
{
	qint64 actsize = 0;
	while (maxsize > 0 && rxavail > 0) {
		Q_ASSERT(!rxsegs.isEmpty());
		RxSegment &seg = rxsegs.first();

		if (maxsize < seg.datalen) {
			// Reading just part of a segment.
			qDebug() << "read partial segment" << maxsize
				<< "of" << seg.datalen;
			memcpy(data, seg.buf.data() + seg.hdrlen, maxsize);
			seg.hdrlen += maxsize;
			seg.datalen -= maxsize;
			rxavail -= maxsize;
			actsize += maxsize;
			Q_ASSERT(rxavail >= 0);
			break;
		}

		int size = seg.datalen;
		Q_ASSERT(size >= 0);
		memcpy(data, seg.buf.data() + seg.hdrlen, size);
		data += size;

		actsize += size;
		maxsize -= size;

		rxavail -= size;
		if (seg.flags & FIN) {
			endread = true;
			rxavail--;
			qDebug() << "readData: FIN, rxavail" << rxavail;
			Q_ASSERT(rxavail >= 0);
		}

		Q_ASSERT(rxavail >= 0);
		rxsegs.removeFirst();
	}
	return actsize;
}

void TcpFlow::flushAck()
{
	if (rxunacked) {
		rxunacked = 0;
		txControl(ACK, txseq, rxseq);
		acktimer.stop();
	}
}

void TcpFlow::ackTimeout()
{
	flushAck();
	Q_ASSERT(!acktimer.isActive());
}

void TcpFlow::rtxTimeout(bool fail)
{
	qDebug() << "Flow::rtxTimeout" << (fail ? "- FAILED" : "")
		<< "period" << rtxtimer.interval();
	if (state == TimeWait)
		return goClosed();
	if (state == Closed)
		return rtxtimer.stop();

	// Restart the retransmission timer
	// with an exponentially increased backoff delay.
	rtxtimer.restart();

	// Reset cwnd and go back to slow start
	ssthresh = qMax(flightsize / 2, cwndMin);
	cwnd = cwndMin;
	//qDebug("rtxTimeout: ssthresh = %d, cwnd = %d", ssthresh, cwnd);

	// Pull everything off of rtxq and put it back on txq for retransmit.
	// This brings flightsize back to zero,
	// which ensures we'll definitely be able to transmit again.
	while (!rtxq.isEmpty()) {
		TxSegment &seg = rtxq.first();
		txInsert(txq, seg);
		flightsize -= seg.datalen;
		Q_ASSERT(flightsize >= 0);
		rtxq.removeFirst();
	}
	Q_ASSERT(flightsize == 0);

	// Start retransmission.
	tryTransmit();

	// If we exceed a threshold timeout, signal a failed connection.
	// The subclass has no obligation to do anything about this, however.
	if (fail) {
		//qDebug() << "rtxTimeout: failed at time"
		//	<< QDateTime::currentDateTime()
		//		.toString("h:mm:ss:zzz")
		//	<< "failtime" << rtxtimer.failTime()
		//		.toString("h:mm:ss:zzz");
		goClosed();
	}
}

// Enter the TimeWait state and start the TIME-WAIT period.
// We just reuse the retransmit timer for this purpose.
void TcpFlow::goTimeWait()
{
	//qDebug() << "Entering TimeWait state";
	state = TimeWait;
	// XXX rtxtimer.start(2*2*60*1000000); // 2xMSL, where MSL = 2 minutes
	rtxtimer.start(5*1000000);	// abbreviated for testing...
}

// Enter the Closed state and notify our stream of disconnection.
// If the stream no longer exists, then self-destruct.
void TcpFlow::goClosed()
{
	//qDebug() << "Entering Closed state";
	state = Closed;
	if (strm)
		strm->disconnect();
	else
		deleteLater();
}


////////// TcpStream //////////

TcpStream::TcpStream(Host *host, QObject *parent)
:	QIODevice(parent),
	flow(new TcpFlow(host, this))
{
}

TcpStream::~TcpStream()
{
	close();
	if (flow->state == flow->Closed)
		delete flow;
	// otherwise let it linger until it's fully closed...
}

void TcpStream::connectToHost(const QHostAddress &addr, quint16 port)
{
	flow->connectToHost(addr, port);
}

bool TcpStream::atEnd() const
{
	return flow->endread;
}

qint64 TcpStream::bytesAvailable() const
{
	return flow->rxavail;
}

qint64 TcpStream::readData(char *data, qint64 maxSize)
{
	if (flow->endread)
		return disconnected(), 0;
	return flow->readData(data, maxSize);
}

qint64 TcpStream::writeData(const char *data, qint64 maxSize)
{
	return flow->writeData(data, maxSize);
}

void TcpStream::close()
{
	endRead();
	endWrite();
}

void TcpStream::disconnect()
{
	setOpenMode(QIODevice::NotOpen);
	disconnected();
}


////////// TcpServer //////////

TcpServer::TcpServer(Host *host, QObject *parent)
:	SocketReceiver(host, parent),
	host(host),
	lisn(false)
{
	bind(TCP_MAGIC);
}

void TcpServer::listen()
{
	lisn = true;
}

void TcpServer::receive(QByteArray &pkt, XdrStream &,
				const SocketEndpoint &src)
{
	// Create a stream if it's a SYN packet and we're listening
	TcpHeader *h = (TcpHeader*)pkt.data();
	quint16 flags = ntohs(h->flags);
	if (!(flags & SYN) || !lisn) {
		qDebug("Unrecognized non-SYN TCP packet");
		return; // XXX respond with RST
	}
	//qDebug() << "Accepting connection from" << src.toString();

	TcpStream *strm = new TcpStream(host, this);
	strm->flow->listen(src);
	strm->flow->receive(pkt, src);
	newConnection(strm);
}

