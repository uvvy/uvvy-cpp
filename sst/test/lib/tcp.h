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
#ifndef SST_TEST_TCP_H
#define SST_TEST_TCP_H

#include <QHash>
#include <QQueue>
#include <QIODevice>

#include "timer.h"
#include "sock.h"


namespace SST {

class TcpFlow;
class TcpStream;
class TcpServer;
class Host;


// 32-bit wraparound sequence numbers used in TCP
struct TcpSeq
{
	qint32 v;

	// Constructors
	inline TcpSeq() { }
	inline TcpSeq(qint32 v) : v(v) { }
	inline TcpSeq(const TcpSeq &o) : v(o.v) { }

	// Assignment
	inline TcpSeq &operator=(const TcpSeq &o) { v = o.v; return *this; }

	// Sequence number comparison
	inline bool operator==(const TcpSeq &o) const { return v == o.v; }
	inline bool operator!=(const TcpSeq &o) const { return v != o.v; }
	inline bool operator<=(const TcpSeq &o) const { return (o.v-v) >= 0; }
	inline bool operator< (const TcpSeq &o) const { return (o.v-v) > 0; }
	inline bool operator>=(const TcpSeq &o) const { return (o.v-v) <= 0; }
	inline bool operator> (const TcpSeq &o) const { return (o.v-v) < 0; }

	// Difference between two sequence numbers
	inline qint32 operator-(const TcpSeq &o) const { return (v-o.v); }

	// Addition or subtraction of a delta to a sequence number
	inline TcpSeq operator+(qint32 d) const { return TcpSeq(v+d); }
	inline TcpSeq operator-(qint32 d) const { return TcpSeq(v-d); }

	// In-place delta addition or subtraction
	inline TcpSeq &operator+=(qint32 d) { v += d; return *this; }
	inline TcpSeq &operator-=(qint32 d) { v -= d; return *this; }
};


// This internal class actually implements the TCP state machine.
// We separate it from TcpStream so that it can linger on as necessary
// after the TcpStream object itself has been destroyed.
class TcpFlow : public SocketFlow
{
	friend class TcpStream;
	friend class TcpServer;
	Q_OBJECT

private:
	// Sender's maximum segment size.  XXX should be dynamic.
	static const int smss = 1200;

	// Congestion window limits
	static const int cwndMin = 2*smss;
	static const int cwndMax = 1<<30;

	// Round-trip time estimation parameters
	static const int rttInit = 500*1000;	// Initial RTT estimate: 1/2s
	static const int rttMax = 10*1000*1000;	// Max round-trip time: 10s

	// Threshold at which to start fast retransmit and fast recovery.
	// XXX should be 3 according to RFCs (variable according to research),
	// but make it 1 for now for a fair comparision with SST,
	// which at the moment doesn't have a dupthresh at all...
	static const int dupthresh = 1;

	// Minimum and maximum size of a TCP header including options.
	static const int minHdr = 20;
	static const int maxHdr = 60;

	// Default receiver window size
	static const int defaultWindow = 65535;

	// Maximum time we wait before sending a delayed ACK
	static const int ackDelay = 10*1000;	// 10 milliseconds (1/100 sec)


	// Session info
	Host *host;
	QPointer<TcpStream> strm;	// Back pointer to stream object
	SocketEndpoint remoteep;

	// TCP state machine
	enum State {
		Closed = 0,
		Listen,
		SynSent,
		SynRcvd,
		Estab,
		FinWait1,
		FinWait2,
		Closing,
		CloseWait,
		LastAck,
		TimeWait,
	} state;
	bool lisn;
	bool endread;		// We've seen or forced EOF on read
	bool endwrite;		// We've written our EOF marker

	// Transmit state

	struct TxSegment {
		TcpSeq tsn;			// Logical byte position
		QByteArray buf;			// Packet incl TCP header
		int datalen;			// Size of data payload
		quint16 flags;			// Flags for packet header
		//bool sacked;			// Got a SACK for this segment
	};

	QQueue<TxSegment> txq;	// Transmit queue, sorted by tsn
	TcpSeq txasn;		// Next sequence number to be assigned
	TcpSeq txlim;		// First sequence number beyond receive window
	TcpSeq txseq;		// First never-transmitted sequence number
	TcpSeq ackseq;		// First not-yet-acknowledged sequence number
	TcpSeq markseq;		// Transmit sequence number of "marked" packet
	Time marktime;		// Time at which marked packet was sent
	//quint32 markacks;	// Number of ACK'd packets since last mark
	//quint32 marksent;	// Number of ACKs expected after last mark
	int dupcnt;		// Duplicate ACK counter for fast retransmit

	// TCP congestion control
	int ssthresh;		// Slow start threshold
	int cwnd;		// Congestion window
	bool cwndlim;		// We were cwnd-limited during last round-trip

	// Retransmit state
	Timer rtxtimer;		// Retransmit timer
	QQueue<TxSegment> rtxq; // Segments saved for possible retransmission
	int flightsize;		// Total bytes in rtxq
	bool fastrecov;		// True if we're in fast recovery


	// Receive state

	struct RxSegment {
		TcpSeq rsn;			// Logical byte position
		QByteArray buf;			// Packet buffer incl. headers
		int hdrlen;			// Size of flow + stream hdrs
		int datalen;			// Size of data payload
		quint16 flags;			// Flags from packet header
	};

	typedef QPair<TcpSeq,TcpSeq> RxRange;

	TcpSeq rxseq;		// Cumulative receive seqeunce number
	quint16 rxwin;		// Receive window size
	QList<RxRange> rxsack;	// Higher byte ranges received out-of-order
	QList<RxSegment> rahead;	// Received out of order
	QQueue<RxSegment> rxsegs;	// Received, waiting to be read
	qint64 rxavail;		// Total bytes available in rxsegs
	quint8 rxunacked;	// # contiguous packets not yet ACKed

	// Delayed ACK state
	bool delayack;		// Enable delayed acknowledgments
	Timer acktimer;		// Delayed ACK timer

	// Statistics gathering
	float cumrtt;		// Cumulative measured RTT in milliseconds
	float cumrttvar;	// Cumulative variation in RTT


private:
	TcpFlow(Host *host, TcpStream *strm);
	~TcpFlow();

	void connectToHost(const QHostAddress &addr, quint16 port);

	// Mark a brand-new socket as a listen socket
	void listen(const SocketEndpoint &remoteep);

	// Insert a TxSegment in a queue in order of TSN.
	static void txInsert(QQueue<TxSegment> &q, const TxSegment &seg);

	qint64 writeData (const char *data, qint64 maxSize);
	void tryTransmit();
	bool doTransmit();
	void txSegment(TxSegment &seg, TcpSeq ackno);
	void txControl(quint16 flags, TcpSeq seqno, TcpSeq ackno);

	// Start the retransmit timer with a timeout based on current RTT
	inline void rtxstart()
		{ rtxtimer.start((int)(cumrtt * 2.0)); }

	// Compute the time elapsed since the mark in microseconds.
	qint64 markElapsed();

	void receive(QByteArray &msg, const SocketEndpoint &src);
	void gotSegment(quint16 flags, TcpSeq seqno,
			int hdrlen, int datalen, QByteArray &pkt);
	void flushAck();
	qint64 readData(char *data, qint64 maxSize);

	void endRead();
	void endWrite();
	void goTimeWait();
	void goClosed();

private slots:
	void rtxTimeout(bool fail);
	void ackTimeout();
};

class TcpStream : public QIODevice
{
	friend class TcpFlow;
	friend class TcpServer;
	Q_OBJECT

private:
	TcpFlow *flow;

public:
	TcpStream(Host *host, QObject *parent = NULL);
	~TcpStream();

	// Actively connect to a remote host
	void connectToHost(const QHostAddress &addr, quint16 port);

	// TCP's implementations of QIODevice abstract methods.
	bool atEnd() const;
	qint64 bytesAvailable() const;
	qint64 readData(char *data, qint64 maxSize);
	qint64 writeData (const char *data, qint64 maxSize);

	inline void endRead() { flow->endRead(); }
	inline void endWrite() { flow->endWrite(); }
	void close();

signals:
	void connected();
	void disconnected();
	void error(const QString &);

private:
	void disconnect();
};

class TcpServer : public SocketReceiver
{
	friend class TcpStream;
	Q_OBJECT

private:
	Host *host;

	// Whether we're listening for connections
	bool lisn;


public:
	TcpServer(Host *h, QObject *parent = NULL);

	void listen();

signals:
	void newConnection(TcpStream *stream);

protected:
	virtual void receive(QByteArray &msg, XdrStream &ds,
				const SocketEndpoint &src);

public:
	TcpServer();
	//~TcpServer();
};

} // namespace SST

#endif	// SST_TEST_TCP_H
