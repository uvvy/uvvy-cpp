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
#ifndef KTUPNPMCASTSOCKET_H
#define KTUPNPMCASTSOCKET_H

#include <QSet>
#include <QUdpSocket>

class UPnPRouter;
    
namespace bt
{
    /**
     * @author Joris Guisson
     * 
     * Socket used to discover UPnP devices. This class will keep track
     * of all discovered devices. 
    */
    class UPnPMCastSocket : public QUdpSocket
    {
        Q_OBJECT
    public:
        UPnPMCastSocket(bool verbose = false);
        virtual ~UPnPMCastSocket();
        
        /// Get the number of routers discovered
        uint32_t getNumDevicesDiscovered() const;
        
        /// Find a router using it's server name
        UPnPRouter* findDevice(const QString & name);
        
        /// Save all routers to a file (for convenience at startup)
        void saveRouters(const QString & file);
        
        /// Load all routers from a file
        void loadRouters(const QString & file);

        /// Set verbose mode
        void setVerbose(bool v);
        
    public slots:
        /**
         * Try to discover a UPnP device on the network.
         * A signal will be emitted when a device is found.
         */
        void discover();
    
    private slots:
        void onReadyRead();
        void error(QAbstractSocket::SocketError err);
        void onXmlFileDownloaded(UPnPRouter* r,bool success);
        
    signals:
        /**
         * Emitted when a router or internet gateway device is detected.
         * @param router The router
         */
        void discovered(UPnPRouter* router);

    private:
        class UPnPMCastSocketPrivate;
        UPnPMCastSocketPrivate* d;
    };
}

#endif
