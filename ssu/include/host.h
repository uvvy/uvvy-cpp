#pragma once

#include "link.h"

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
class host : public link_host_state
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
