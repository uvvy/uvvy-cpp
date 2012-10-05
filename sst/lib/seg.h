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
#ifndef SST_SEG_H
#define SST_SEG_H

#include <QQueue>	// XXX FlowSegment

#include "flow.h"
#include "key.h"


namespace SST {

// New segmentable Flow Layer stuff
class FlowSocket;

// Control chunk magic value for the flow protocol ('flw')
static const quint32 flow_seg_magic = 0x00464c57;	// XXX in FlowProtocol

class FlowSegment : public Flow
{
	friend class FlowSocket;
	friend class FlowResponder;
	Q_OBJECT

	static const int qlim_default = 50;	// XX ??

private:
	FlowSegment *other;	// Seg to which we're joined, if middlebox
	FlowSocket *fsock;	// Virtual socket we implement, if not middlebox

	// Queue of received packets for segment joining
	struct RxPkt {
		quint64 rxseq;
		QByteArray pkt;
	};
	QQueue<RxPkt> rxq;
	int qlim;

private:
	virtual bool flowReceive(qint64 pktseq, QByteArray &pkt);

	bool initiateTo(Socket *sock, const Endpoint &remoteep);

private slots:
	void gotReadyTransmit();

public:
	FlowSegment(Host *host, QObject *parent = NULL);

	inline int rxQueuedPackets() { return rxq.size(); }
};


class FlowSocket : public Socket	// arrrgh!!! naming hell...
{
	friend class FlowSegment;
	friend class FlowResponder;
	Q_OBJECT

private:
	Host *const h;
	FlowSegment *fseg;		// first flow segment
	SocketEndpoint peer;		// final end-to-end peer

private:
	virtual bool send(const Endpoint &ep, const char *data, int size);

	virtual bool isCongestionControlled(const Endpoint &ep);
	virtual int mayTransmit(const Endpoint &ep);

	// Override bindFlow to allow only flows to our single peer
	virtual bool bindFlow(const Endpoint &remoteep, Channel localchan,
				SocketFlow *flow);

	// XXX these make no sense here; need to make flow stuff standalone
	// and independent of socket notion
	virtual bool bind(const QHostAddress &addr = QHostAddress::Any,
		quint16 port = 0,
		QUdpSocket::BindMode mode = QUdpSocket::DefaultForPlatform);
	virtual QList<Endpoint> localEndpoints();
	virtual quint16 localPort();
	virtual QString errorString();

public:
	FlowSocket(Host *host, const QHostAddress &peer,
			QObject *parent = NULL);
	~FlowSocket();

	FlowSegment *initiateTo(const Endpoint &remoteep);

	inline Host *host() { return h; }

signals:
	void readyTransmitToPeer();

private slots:
	void keyCompleted(bool success);
};


class FlowResponder : public KeyResponder
{
	Q_OBJECT

	Endpoint targetep;
	FlowSocket *fsock;

public:
	FlowResponder(Host *h);

	void forwardTo(const Endpoint &targetep);
	void forwardUp(FlowSocket *fsock);

	FlowSegment *lastiseg, *lastoseg; // XXX hack for monitoring purposes

private:
	virtual Flow *newFlow(const SocketEndpoint &epi, const QByteArray &idi,
				const QByteArray &, QByteArray &);
};


} // namespace SST

#endif	// SST_SEG_H
