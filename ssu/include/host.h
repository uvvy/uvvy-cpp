//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "dh.h"
#include "link.h"
#include "timer.h"
#include "negotiation/key_responder.h"

namespace ssu {

/**
 * This class encapsulates all per-host state used by the ssu protocol.
 * By centralizing this state here instead of using global/static variables,
 * the host environment can be virtualized for simulation purposes
 * and multiple ssu instances can be run in one process.
 * 
 * It is the client's responsibility to ensure that a host object
 * is not destroyed while any ssu objects still refer to it.
 */
class host
    : public link_host_state
    , public dh_host_state
    , public key_host_state
    , public timer_host_state
{
public:
	/**
	 * Create a "bare-bones" host state object with no links or identity.
	 * Client must establish a host identity via set_host_ident()
	 * and activate one or more network links before using ssu.
	 */
	host();
};

}
