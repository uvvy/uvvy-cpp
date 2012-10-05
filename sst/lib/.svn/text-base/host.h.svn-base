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
#ifndef SST_HOST_H
#define SST_HOST_H

#include <QList>
#include <QHash>

#include "timer.h"
#include "sock.h"
#include "ident.h"
#include "dh.h"
#include "key.h"
#include "regcli.h"
#include "stream.h"


namespace SST {

/** This class encapsulates all per-host state used by the SST protocol.
 * By centralizing this state here instead of using global/static variables,
 * the host environment can be virtualized for simulation purposes
 * and multiple SST instances can be run in one process.
 * 
 * It is the client's responsibility to ensure that a Host object
 * is not destroyed while any SST objects still exist that refer to it.
 */
class Host :	public TimerHostState,
		public SocketHostState,
		public IdentHostState,
		public DHHostState,
		public KeyHostState,
		public RegHostState,
		public StreamHostState
{
public:
	/** Create a "bare-bones" host state object with no links or identity.
	 * Client must establish a host identity via setHostIdent()
	 * and activate one or more network links before using SST. */
	Host();

	/** Create an easy-to-use default Host object.
	 * Uses the provided QSettings registry to locate,
	 * or create if necessary, a persistent host identity,
	 * as described for IdentHostState::initHostIdent().
	 * Also creates and binds to at least one UDP link,
	 * using a UDP port number specified in the QSettings,
	 * or defaulting to @a defaultUdpPort if none.
	 * If the desired UDP port cannot be bound,
	 * just picks an arbitrary UDP port instead.
	 */
	Host(QSettings *settings, quint16 defaultUdpPort);


	virtual Host *host();

	// stream layer listeners, targets, responder

public:
};

} // namespace SST

#endif	// SST_HOST_H
