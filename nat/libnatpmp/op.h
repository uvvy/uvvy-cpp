// op.h
// Author: Allen Porter <allen@thebends.org>

#include <netinet/in.h>

namespace pmp {

class Op {
 public:
  virtual ~Op() { }

  virtual bool Run(const struct sockaddr_in* name,
                   const char* request, size_t request_size,
                   char* response, size_t response_size) = 0;

 protected:
  Op() { }
};

// Returns a new udp operation
//
Op* NewUdpOp();

}  // namespace pmp
