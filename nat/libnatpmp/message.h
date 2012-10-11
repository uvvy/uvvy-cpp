// messages.h
// Author: Allen Porter <allen@thebends.org>

namespace pmp {

#define UPMP_PORT 5351

enum ResultCode {
  SUCCESS = 0,
  UNSUPPORTED_VERSION = 1,
  NOT_AUTHORIZED = 2,
  NETWORK_FAILURE = 3,
  OUT_OF_RESOURCES = 4,
  UNSUPPORTED_OPCODE = 5
};

struct request_header {
  uint8_t vers;
  uint8_t op;
};

struct response_header {
  uint8_t vers;
  uint8_t op;
  uint16_t result;
  uint32_t epoch;
};

struct public_ip_request {
  struct request_header header;
};

struct public_ip_response {
  struct response_header header;
  struct in_addr ip;
};

struct map_request {
  // op: UDP=1, TCP=2
  struct request_header header;
  uint16_t reserved;  // must be zero
  uint16_t private_port;
  uint16_t public_port;
  uint32_t lifetime;  // seconds
};

struct map_response {
  struct response_header header;
  uint16_t private_port;
  uint16_t public_port;
  uint32_t lifetime;  // seconds
};

}  // namespace pmp
