// util.cpp
// Author: Allen Porter <allen@thebends.org>
#include "util.h"

#include <assert.h>
#include <err.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include "message.h"
#include "op.h"
#include "route.h"

#include <stdio.h>

namespace pmp {

bool GetGatewayInfo(in_addr* public_ip, uint32_t* uptime) {
  struct sockaddr_in name;
  if (!GetPortMapAddress(&name)) {
    return false;
  }

  Op* op = NewUdpOp();

  struct public_ip_request request;
  bzero(&request, sizeof(struct public_ip_request));
  request.header.vers = 0;
  request.header.op = 0;
  struct public_ip_response resp;
  if (!op->Run(&name, (const char*)&request,
               sizeof(struct public_ip_request),
               (char*)&resp, sizeof(struct public_ip_response))) {
    return false;
  }
  if (resp.header.vers != 0) {
    warn("unsupported version: %d", resp.header.vers);
    return false;
  } else if (resp.header.op != 128) {
    warn("unexpected server result: %d", resp.header.result);
    return false;
  }
  public_ip->s_addr = resp.ip.s_addr;
  *uptime = ntohl(resp.header.epoch);
  return true;
}

bool CreateMap(Proto proto,
               uint16_t private_port,
               uint16_t* public_port,
               uint32_t* lifetime) {
  struct sockaddr_in name;
  if (!GetPortMapAddress(&name)) {
    return false;
  }

  Op* op = NewUdpOp();

  struct map_request request;
  bzero(&request, sizeof(struct map_request));
  request.header.vers = 0;
  request.header.op = proto; 
  request.reserved = 0;
  request.private_port = ntohs(private_port);
  request.public_port = ntohs(*public_port);
  request.lifetime = ntohl(*lifetime);

printf("request.op=%d\n", request.header.op);
  struct map_response resp;
  if (!op->Run(&name, (const char*)&request,
               sizeof(struct map_request),
               (char*)&resp, sizeof(struct map_response))) {
    return false;
  }
  if (resp.header.vers != 0) {
    warn("unsupported version: %d", resp.header.vers);
    return false;
  } else if (resp.header.op != (128 + proto)) {
    warn("unexpected server result: %d", resp.header.result);
    return false;
  }
printf("resp.result:%d\n", resp.header.result);
printf("resp.epoch:%u\n", ntohl(resp.header.epoch));
printf("resp.public_port:%d\n", ntohs(resp.public_port));
printf("resp.private_port:%d\n", ntohs(resp.private_port));
  if (private_port != ntohs(resp.private_port)) {
    warn("unexpected private_port mismatch (%d != %d)", private_port,
         ntohs(resp.private_port));
    return false;
  }
  *public_port = ntohs(resp.public_port);
  *lifetime = ntohl(resp.lifetime);
  return true;
}

}  // namespace pmp

