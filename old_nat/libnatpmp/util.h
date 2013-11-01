// util.h
// Author: Allen Porter <allen@thebends.org>

#ifndef __PMP_UTIL_H__
#define __PMP_UTIL_H__

#include <netinet/in.h>

namespace pmp {

// Attempts to get the public ip address and uptime of the gateway.  Returns
// true if successful, and false otherwise.
bool GetGatewayInfo(in_addr* public_ip, uint32_t* uptime);

// Creates a port forward map from the specified public port to our
// the specified private port of this machine.

enum Proto {
  UDP = 1,
  TCP = 2
};

bool CreateMap(Proto proto,
               uint16_t private_port,
               uint16_t* public_port,
               uint32_t* lifetime);

}  // namespace pmp

#endif  // __PMP_UTIL_H__
