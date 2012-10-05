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


#include <lmcons.h>

#include <QUdpSocket>
#include <QHostAddress>
#include <QtDebug>

#include "os.h"

using namespace SST;


QString SST::userName()
{
	Q_ASSERT(sizeof(TCHAR) == sizeof(ushort));
	TCHAR winUserName[UNLEN + 1]; // UNLEN is defined in LMCONS.H
	DWORD winUserNameSize = UNLEN + 1;
	GetUserName(winUserName, &winUserNameSize);
#if defined(UNICODE)
	return QString::fromUtf16((ushort*)winUserName);
#else // not UNICODE
	return QString::fromLocal8Bit(winUserName);
#endif // not UNICODE
}

QList<QHostAddress> SST::localHostAddrs()
{
	QUdpSocket sock;
	if (!sock.bind())
		qFatal("Can't bind local UDP socket");
	int sockfd = sock.socketDescriptor();
	Q_ASSERT(sockfd >= 0);

	// Get the local host's interface list from Winsock via WSAIoctl().
	// Since we have no way of knowing how big a buffer we need,
	// retry with progressively larger buffers until we succeed.
	QByteArray buf;
	buf.resize(sizeof(SOCKET_ADDRESS_LIST)*2);
	int retries = 0;
	forever {
		DWORD actsize;
		int rc = WSAIoctl(sockfd, SIO_ADDRESS_LIST_QUERY, NULL, 0,
				buf.data(), buf.size(), &actsize, NULL, NULL);
		if (rc == 0)
			break;

		buf.resize(buf.size() * 2);
		if (++retries > 20)
			qFatal("Can't find local host's IP addresses: %d",
				WSAGetLastError());
	}

	// Parse the returned address list.
	SOCKET_ADDRESS_LIST *salist = (SOCKET_ADDRESS_LIST*)buf.data();
	QList<QHostAddress> qaddrs;
	for (int i = 0; i < salist->iAddressCount; i++) {
		sockaddr *sa = salist->Address[i].lpSockaddr;
		QHostAddress qa(sa);
		if (qa.isNull())
			continue;
		qaddrs.append(qa);
		//qDebug() << "Local IP address:" << qa.toString();
	}

	return qaddrs;
}

