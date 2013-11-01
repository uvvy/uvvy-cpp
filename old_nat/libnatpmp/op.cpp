// op.cpp
// Author: Allen Porter <allen@thebends.org>
#include "op.h"

#include <assert.h>
#include <arpa/inet.h>
#include <err.h>
#include <limits.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace pmp {
/*
static bool SetNonBlocking(int sock) {
  int flags;
  if ((flags = fcntl(sock, F_GETFL, 0)) == -1)
    flags = 0;
  return (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1);
}
*/

class UdpOp : public Op {
 public:
  UdpOp() { }

  virtual bool Run(const struct sockaddr_in* name,
                   const char* request, size_t request_size,
                   char* response, size_t response_size) {
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      warn("socket");
      return false;
    }

    ssize_t nsent;
    if ((nsent = sendto(sock, request, request_size, 0,
                        (const struct sockaddr*)name,
                        sizeof(struct sockaddr_in))) == -1) {
      warn("sendto");
      return false;
    }

    ssize_t nread;
    struct sockaddr_in recv_name;
    socklen_t namelen;
    if ((nread = recvfrom(sock, response, response_size,
                          0, (struct sockaddr*)&recv_name, &namelen)) == -1) {
      warn("recvfrom");
      return false;
    }
    if ((size_t)nread != response_size) {
      warn("Only read %zu byte (expected %zu)", nread, response_size);
      return false;
    }
    assert(namelen = sizeof(struct sockaddr_in));
    return true;
  }
};

Op* NewUdpOp() {
  return new UdpOp();
}

}  // namespace pmp
