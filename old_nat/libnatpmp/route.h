// route.h
// Author: Allen Porter <allen@thebends.org>
#ifndef __PMP_ROUTE_H__
#define __PMP_ROUTE_H__

#include <arpa/inet.h>

namespace pmp {

// Returns the local IP address of the default route.
bool GetDefaultGateway(in_addr* addr);

// Returns the local IP address and port of the Port Pam Protocol address of
// the default gateway.  This is the address and port that should be used to
// send port map requests.
bool GetPortMapAddress(struct sockaddr_in* name);


}  // pmp

#endif  // __PMP_ROUTE_H__
