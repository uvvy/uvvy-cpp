/***************************************************************************
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include <QtDebug>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef Q_WS_WIN
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#endif
#include <arpa/inet.h>
#include <QUrl>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include "upnpmcastsocket.h"


namespace bt
{
	static bool UrlCompare(const QUrl & a, const QUrl & b)
	{
		return a == b;
		// if (a == b)
		// 	return true;
		
		// return 
		// 	a.protocol() == b.protocol() && 
		// 	a.host() == b.host() &&
		// 	a.pass() == b.pass() &&
		// 	a.port(80) == b.port(80) &&
		// 	a.encodedPathAndQuery() == b.encodedPathAndQuery();
	}
	
	class UPnPMCastSocket::UPnPMCastSocketPrivate
	{
	public:
		UPnPMCastSocketPrivate(bool verbose);
		~UPnPMCastSocketPrivate();
		
		UPnPRouter* parseResponse(const QByteArray & arr);
		void joinUPnPMCastGroup(int fd);
		void leaveUPnPMCastGroup(int fd);
		void onXmlFileDownloaded(UPnPRouter* r,bool success);
		UPnPRouter* findDevice(const QUrl & location);
		
		QSet<UPnPRouter*> routers;
		QSet<UPnPRouter*> pending_routers; // routers which we are downloading the XML file from
		bool verbose;
	};
	
	UPnPMCastSocket::UPnPMCastSocket(bool verbose) : d(new UPnPMCastSocketPrivate(verbose))
	{
        qDebug() << __PRETTY_FUNCTION__;
		QObject::connect(this,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
		QObject::connect(this,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));
	
		for (uint32_t i = 0;i < 10;i++)
		{
			if (!bind(1900 + i,QUdpSocket::ShareAddress))
				qWarning() << "Cannot bind to UDP port 1900: " << errorString();
			else
				break;
		}
		
		d->joinUPnPMCastGroup(socketDescriptor());
	}
	
	
	UPnPMCastSocket::~UPnPMCastSocket()
	{
        qDebug() << __PRETTY_FUNCTION__;
		d->leaveUPnPMCastGroup(socketDescriptor());
		delete d;
	}
	
	void UPnPMCastSocket::discover()
	{
		qDebug() << "Trying to find UPnP devices on the local network";
		
		// send a HTTP M-SEARCH message to 239.255.255.250:1900
		const char* data = "M-SEARCH * HTTP/1.1\r\n" 
				"HOST: 239.255.255.250:1900\r\n"
				"ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
				"MAN:\"ssdp:discover\"\r\n"
				"MX:3\r\n"
				"\r\n\0";
		
		if (d->verbose)
		{
			qDebug() << "Sending: " << data;
		}
		
		writeDatagram(data,strlen(data),QHostAddress("239.255.255.250"),1900);
	}
	
	void UPnPMCastSocket::onXmlFileDownloaded(UPnPRouter* r,bool success)
	{
		qDebug() << __PRETTY_FUNCTION__;

		d->pending_routers.remove(r);
		if (!success)
		{
			// we couldn't download and parse the XML file so 
			// get rid of it
			r->deleteLater();
		}
		else
		{
			// add it to the list and emit the signal
			QUrl location = r->getLocation();
			if (d->findDevice(location))
			{
				r->deleteLater();
			}
			else
			{
				d->routers.insert(r);
				discovered(r);
			}
		}
	}
	
	void UPnPMCastSocket::onReadyRead()
	{
		qDebug() << __PRETTY_FUNCTION__;

		if (pendingDatagramSize() == 0)
		{
			qDebug() << "0 byte UDP packet ";
			// KDatagramSocket wrongly handles UDP packets with no payload
			// so we need to deal with it oursleves
			int fd = socketDescriptor();
			char tmp;
			::read(fd,&tmp,1);
			return;
		}
		
		QByteArray data(pendingDatagramSize(),0);
		if (readDatagram(data.data(),pendingDatagramSize()) == -1)
			return;
		
		if (d->verbose)
		{
			qDebug() << "Received: " << QString(data);
		}
		
		// try to make a router of it
		UPnPRouter* r = d->parseResponse(data);
		if (r)
		{
			QObject::connect(r,SIGNAL(xmlFileDownloaded(UPnPRouter*,bool)),
					this,SLOT(onXmlFileDownloaded(UPnPRouter*,bool)));
			
			// download it's xml file
			r->downloadXMLFile();
			d->pending_routers.insert(r);
		}
	}
	
	
	
	void UPnPMCastSocket::error(QAbstractSocket::SocketError )
	{
		qWarning() << "UPnPMCastSocket Error : " << errorString();
	}
	
	void UPnPMCastSocket::saveRouters(const QString & file)
	{
		qDebug() << __PRETTY_FUNCTION__;
		QFile fptr(file);
		if (!fptr.open(QIODevice::WriteOnly))
		{
			qWarning() << "Cannot open file " << file << " : " << fptr.errorString();
			return;
		}
		
		// file format is simple : 2 lines per router, 
		// one containing the server, the other the location
		QTextStream fout(&fptr);
		foreach (UPnPRouter* r,d->routers)
		{
			fout << r->getServer() << ::endl;
			fout << r->getLocation().toString() << ::endl;
		}
	}
	
	void UPnPMCastSocket::loadRouters(const QString & file)
	{
		qDebug() << __PRETTY_FUNCTION__;
		QFile fptr(file);
		if (!fptr.open(QIODevice::ReadOnly))
		{
			qWarning() << "Cannot open file " << file << " : " << fptr.errorString();
			return;
		}
		
		// file format is simple : 2 lines per router, 
		// one containing the server, the other the location
		QTextStream fin(&fptr);
		
		while (!fin.atEnd())
		{
			QString server, location;
			server = fin.readLine();
			location = fin.readLine();
				
			UPnPRouter* r = new UPnPRouter(server,location);
			// download it's xml file
			QObject::connect(r,SIGNAL(xmlFileDownloaded(UPnPRouter*,bool)),this,SLOT(onXmlFileDownloaded(UPnPRouter*,bool)));
			r->downloadXMLFile();
			d->pending_routers.insert(r);
		}
	}
	
	uint32_t UPnPMCastSocket::getNumDevicesDiscovered() const 
	{
        qDebug() << __PRETTY_FUNCTION__;
		return d->routers.count();
	}
	
	UPnPRouter* UPnPMCastSocket::findDevice(const QString & name) 
	{
        qDebug() << __PRETTY_FUNCTION__;
		QUrl location(name);
		return d->findDevice(location);
	}
	
	void UPnPMCastSocket::setVerbose(bool v) 
	{
		d->verbose = v;
	}
	
	/////////////////////////////////////////////////////////////
	
	UPnPMCastSocket::UPnPMCastSocketPrivate::UPnPMCastSocketPrivate(bool verbose) : verbose(verbose)
	{
        qDebug() << __PRETTY_FUNCTION__;
	}
	
	UPnPMCastSocket::UPnPMCastSocketPrivate::~UPnPMCastSocketPrivate()
	{
        qDebug() << __PRETTY_FUNCTION__;
		qDeleteAll(pending_routers);
		qDeleteAll(routers);
	}
	
	void UPnPMCastSocket::UPnPMCastSocketPrivate::joinUPnPMCastGroup(int fd)
	{
        qDebug() << __PRETTY_FUNCTION__;
		struct ip_mreq mreq;
		memset(&mreq,0,sizeof(struct ip_mreq));
		
		inet_aton("239.255.255.250",&mreq.imr_multiaddr);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		
#ifndef Q_WS_WIN
		if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(struct ip_mreq)) < 0)
#else
		if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(struct ip_mreq)) < 0)
#endif
		{
			qDebug() << "Failed to join multicast group 239.255.255.250";
		} 
	}
	
	void UPnPMCastSocket::UPnPMCastSocketPrivate::leaveUPnPMCastGroup(int fd)
	{
        qDebug() << __PRETTY_FUNCTION__;
		struct ip_mreq mreq;
		memset(&mreq,0,sizeof(struct ip_mreq));
		
		inet_aton("239.255.255.250",&mreq.imr_multiaddr);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		
#ifndef Q_WS_WIN
		if (setsockopt(fd,IPPROTO_IP,IP_DROP_MEMBERSHIP,&mreq,sizeof(struct ip_mreq)) < 0)
#else
		if (setsockopt(fd,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mreq,sizeof(struct ip_mreq)) < 0)
#endif
		{
			qDebug() << "Failed to leave multicast group 239.255.255.250";
		} 
	}
	
	UPnPRouter* UPnPMCastSocket::UPnPMCastSocketPrivate::parseResponse(const QByteArray & arr)
	{
        qDebug() << __PRETTY_FUNCTION__;
		QStringList lines = QString::fromAscii(arr).split("\r\n");
		QString server;
		QUrl location;
		
		
		qDebug() << "Received:";
		for (uint32_t idx = 0;idx < lines.count(); idx++)
			qDebug() << lines[idx];
		
		
		// first read first line and see if contains a HTTP 200 OK message
		QString line = lines.first();
		if (!line.contains("HTTP"))
		{
			// it is either a 200 OK or a NOTIFY
			if (!line.contains("NOTIFY") && !line.contains("200")) 
				return 0;
		}
		else if (line.contains("M-SEARCH")) // ignore M-SEARCH 
			return 0;
		
		// quick check that the response being parsed is valid 
		bool validDevice = false; 
		for (int idx = 0;idx < lines.count() && !validDevice; idx++) 
		{ 
			line = lines[idx]; 
			if ((line.contains("ST:") || line.contains("NT:")) && line.contains("InternetGatewayDevice")) 
			{
				validDevice = true; 
			}
		} 
		if (!validDevice)
		{
			qWarning() << "Not a valid Internet Gateway Device";
			return 0; 
		}
		
		// read all lines and try to find the server and location fields
		for (int i = 1;i < lines.count();i++)
		{
			line = lines[i];
			if (line.startsWith("Location") || line.startsWith("LOCATION") || line.startsWith("location"))
			{
				location = line.mid(line.indexOf(':') + 1).trimmed();
				if (!location.isValid())
					return 0;
			}
			else if (line.startsWith("Server") || line.startsWith("server") || line.startsWith("SERVER"))
			{
				server = line.mid(line.indexOf(':') + 1).trimmed();
				if (server.length() == 0)
					return 0;
				
			}
		}
		
		if (findDevice(location))
		{
			return 0;
		}
		else
		{
			qDebug() << "Detected IGD " << server;
			// everything OK, make a new UPnPRouter
			return new UPnPRouter(server,location,verbose); 
		}
	}
	
	UPnPRouter* UPnPMCastSocket::UPnPMCastSocketPrivate::findDevice(const QUrl& location)
	{
        qDebug() << __PRETTY_FUNCTION__;
		foreach (UPnPRouter* r, routers)
		{
			if (UrlCompare(r->getLocation(),location))
				return r;
		}
		
		return 0;
	}


}

