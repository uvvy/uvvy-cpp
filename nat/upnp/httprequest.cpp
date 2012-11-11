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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include <QTimer>
#include <QHostAddress>
#include <QHttpResponseHeader>
#include <QStringList>
#include "httprequest.h"




namespace bt
{

	HTTPRequest::HTTPRequest(const QString & hdr,const QString & payload,const QString & host,uint16_t port,bool verbose) 
		: hdr(hdr),payload(payload),verbose(verbose),host(host),port(port),finished(false),success(false)
	{
		sock = new QTcpSocket(this);
		connect(sock,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
		connect(sock,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(onError(QAbstractSocket::SocketError)));
		connect(sock,SIGNAL(connected()),this, SLOT(onConnect()));
	}
	
	
	HTTPRequest::~HTTPRequest()
	{
		sock->close();
	}
	
	void HTTPRequest::start()
	{
		success = false;
		finished = false;
		reply.clear();
		sock->connectToHost(host,port);
		QTimer::singleShot(30000,this,SLOT(onTimeout()));
	}
	
	void HTTPRequest::cancel()
	{
		finished = true;
		sock->close();
	}
	
	void HTTPRequest::onConnect()
	{
		if (finished)
			return;
		
		payload = payload.replace("$LOCAL_IP",sock->localAddress().toString());
		hdr = hdr.replace("$CONTENT_LENGTH",QString::number(payload.length()));
			
		QString req = hdr + payload;
		if (verbose)
		{
			qDebug() << "Sending " << endl;
			QStringList lines = hdr.split("\r\n");
			foreach (const QString &line,lines)
				qDebug() << line << endl;
			
			qDebug() << payload << endl;
		}

		sock->write(req.toAscii());
	}
	
	void HTTPRequest::onReadyRead()
	{
		if (finished)
			return;
		
		uint32_t ba = sock->bytesAvailable();
		if (ba == 0)
		{
			if (!finished)
			{
				error = ("Connection closed unexpectedly");
				success = false;
				finished = true;
				result(this);
				// operationFinished(this);
			}
			sock->close();
			return;
		}
		
		reply.append(sock->read(ba));
		int eoh = reply.indexOf("\r\n\r\n");
		if (eoh != -1)
		{
			reply_header = QHttpResponseHeader(QString::fromAscii(reply.mid(0,eoh + 4)));
			if (reply_header.contentLength() > 0 && reply.size() < eoh + 4 + reply_header.contentLength())
			{
				// Haven't got full content yet, so return and wait for more
				return;
			}
			else
			{
				reply = reply.mid(eoh + 4);
				success = reply_header.statusCode() == 200;
				if (!success)
					error = reply_header.reasonPhrase();
				finished = true;
				result(this);
				// operationFinished(this);
			}
		}
		else
			return;
	}
	
	void HTTPRequest::onError(QAbstractSocket::SocketError err)
	{
		Q_UNUSED(err);
		if (finished)
			return;
		
		qDebug() << "HTTPRequest error : " << sock->errorString() << endl;
		success = false;
		finished = true;
		sock->close();
		error = sock->errorString();
		result(this);
		// operationFinished(this);
	}
	
	void HTTPRequest::onTimeout()
	{
		if (finished)
			return;

		qDebug() << "HTTPRequest timeout" << endl;
		success = false;
		finished = true;
		sock->close();
		error = ("Operation timed out");
		result(this);
		// operationFinished(this);
	}


}
