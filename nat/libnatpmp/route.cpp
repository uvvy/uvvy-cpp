// route.cpp
//
// Who knew getting the default gateway was such a huge pain?  Hopefully this
// saves someone some time.

#include "route.h"

#include <assert.h>
#include <err.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include "message.h"

namespace pmp {

struct msg {
  struct rt_msghdr m_rtm;
  // Route data is returned in space
  unsigned char space[512];
};

bool GetDefaultGateway(in_addr* addr) {
  static int seq = 0;

  struct msg m;
  int len = sizeof(struct msg);
  bzero(&m, sizeof(struct msg));
  m.m_rtm.rtm_type = RTM_GET;
  m.m_rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
  m.m_rtm.rtm_version = RTM_VERSION;
  m.m_rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;
  m.m_rtm.rtm_seq = ++seq;
  m.m_rtm.rtm_msglen = len;

  int sock;
  if ((sock = socket(PF_ROUTE, SOCK_RAW, AF_INET)) == -1) {
    warn("socket");
    return false;
  }

  ssize_t nbytes = write(sock, (char*)&m, len);
  if (nbytes == -1) {
    warn("write");
    return false;
  } else if (nbytes != len) {
    warnx("unexpected number of bytes written (%zu != %d)", nbytes, len);
    return false;
  }
  bzero(&m, sizeof(struct msg));

  nbytes = read(sock, &m, sizeof(struct msg));
  if (nbytes == -1) {
    warn("read");
    return false;
  }
  assert(seq == m.m_rtm.rtm_seq);
  assert(m.m_rtm.rtm_pid == getpid());
  if (m.m_rtm.rtm_errno) {
    warnc(m.m_rtm.rtm_errno, "read sock opt");
    return false;
  }
  if ((m.m_rtm.rtm_flags bitand RTF_UP) == 0 ||
      (m.m_rtm.rtm_flags bitand RTF_GATEWAY) == 0) {
    return false;
  }
  if ((m.m_rtm.rtm_addrs bitand RTA_DST) == 0 ||
      (m.m_rtm.rtm_addrs bitand RTA_GATEWAY) == 0) {
    return false;
  }
  if (m.m_rtm.rtm_msglen > len) {
    warnx("msg len mismatch (%d > %d)", m.m_rtm.rtm_msglen, len);
    return false;
  }
  size_t min_len = sizeof(struct rt_msghdr) + 2 * sizeof(struct sockaddr_in);
  if (m.m_rtm.rtm_msglen < min_len) {
    warnx("msg len mismatch (%d > %d)", m.m_rtm.rtm_msglen, len);
    return false;
  }

  // Verify the default route destination looks sane
  struct sockaddr_in* saddr = (struct sockaddr_in*)m.space;
  if (saddr->sin_len != sizeof(struct sockaddr_in)) {
    warnx("invalid default route destination sin_len (%d != %zu)",
          saddr->sin_len, sizeof(struct sockaddr_in));
    return false;
  }
  if (saddr->sin_family != AF_INET) {
    warnx("invalid default route destination sin_family");
    return false;
  }
  if (saddr->sin_addr.s_addr != 0) {
    warnx("invalid default route destination sin_addr");
    return false;
  }

  // Verify the default default looks sane
  saddr++;
  if (saddr->sin_len != sizeof(struct sockaddr_in)) {
    warnx("invalid default gateway sin_len (%d != %zu)", saddr->sin_len,
          sizeof(struct sockaddr_in));
    return false;
  }
  if (saddr->sin_family != AF_INET) {
    warnx("invalid default gateway sin_family");
    return false;
  }
  if (saddr->sin_addr.s_addr == 0) {
    warnx("invalid default gateway sin_addr");
    return false;
  }

  memcpy(addr, &saddr->sin_addr, sizeof(struct in_addr));
  return true;
}

bool GetPortMapAddress(struct sockaddr_in* name) {
  socklen_t namelen = sizeof(struct sockaddr_in);
  bzero(name, namelen);
  if (!GetDefaultGateway(&name->sin_addr)) {
    return false;
  }
  name->sin_family = AF_INET;
  name->sin_port = htons(UPMP_PORT);
  return true;
}

}  // namespace pmp
