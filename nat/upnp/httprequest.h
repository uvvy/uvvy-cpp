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
#ifndef KTHTTPREQUEST_H
#define KTHTTPREQUEST_H

#include <QTcpSocket>
#include <QHttpResponseHeader>


namespace bt
{
    
    /**
     * @author Joris Guisson
     * 
     * Simple HTTP request class. 
     * TODO: switch to KIO for this
     */
    class HTTPRequest : public QObject
    {
        Q_OBJECT
    public:
        /**
         * Constructor, set the url and the request header.
         * @param hdr The http request header
         * @param payload The payload
         * @param host The host
         * @param port THe port
         * @param verbose Print traffic to the log
         */
        HTTPRequest(const QString & hdr,const QString & payload,const QString & host,
                    uint16_t port,bool verbose);
        virtual ~HTTPRequest();
        
        /**
         * Open a connetion and send the request.
         */
        void start();
        
        /**
            Cancel the request, no result signal will be emitted.
        */
        void cancel();
        
        /**
            Get the reply data
        */
        QByteArray replyData() const {return reply;}
        
        /**
            Did the request succeed
        */
        bool succeeded() const {return success;}
        
        /**
            In case of failure this function will return an error string
        */
        QString errorString() const {return error;}
        
    signals:
        /**
         * An OK reply was sent.
         * @param r The sender of the request
         * @param data The data of the reply
         */
        void result(HTTPRequest* r);
        
        
    private slots:
        void onReadyRead();
        void onError(QAbstractSocket::SocketError err);
        void onTimeout();
        void onConnect();
    
    private:
        void parseReply(int eoh);
        
    private:
        QTcpSocket* sock;
        QString hdr,payload;
        bool verbose;
        QString host;
        uint16_t port;
        bool finished;
        QHttpResponseHeader reply_header;
        QByteArray reply;
        bool success;
        QString error;
    };

}

#endif
