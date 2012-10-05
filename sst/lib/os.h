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
//
// OS-specific networking facilities
//
#ifndef SST_NET_H
#define SST_NET_H

class QHostAddress;

#ifdef WIN32	// htonl, ntohl
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif


namespace SST {

// Find the current user's name according to the operating system.
QString userName();

// Find all of the local host's IP addresses (OS specific)
QList<QHostAddress> localHostAddrs();

} // namespace SST

#endif	// SST_NET_H
