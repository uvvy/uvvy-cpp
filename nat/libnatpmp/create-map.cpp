// create-map.cpp
// Author: Allen Porter <allen@thebends.org>
//
// Simple tool that creates port mapping entries in your local NAT PMP gateway.

#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sysexits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include "util.h"
#include "google/gflags.h"

DEFINE_int32(private_port, 0,
             "The private port on this computer used for the mapping.");
DEFINE_int32(public_port, 0,
             "The desired public port (or 0 to be assigned a port).");
DEFINE_bool(udp, false,
            "If true, a port mapping will be made for UDP packets.");
DEFINE_bool(tcp, false,
            "If true, a port mapping will be made for TCP packets.");
DEFINE_int32(lifetime, 3600,
             "The lifetime of the port mapping.");

using namespace std;

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_private_port <= 0) {
    errx(1, "Required flag --private_port was missing");
  } else if (FLAGS_lifetime < 0) {
    errx(1, "Required flag --lifetime was missing");
  } else if (!FLAGS_tcp && !FLAGS_udp) {
    errx(1, "Either --tcp or --udp must be specified");
  } else if (FLAGS_tcp && FLAGS_udp) {
    errx(1, "Only one of --tcp or --udp can be specified");
  }

  uint16_t public_port = FLAGS_public_port;
  uint32_t lifetime = FLAGS_lifetime;
  if (!pmp::CreateMap(FLAGS_udp ? pmp::UDP : pmp::TCP,
                      FLAGS_private_port,
                      &public_port,
                      &lifetime)) {
    errx(1, "pmp::CreateMap failed");
  }

  cout << "Port map created: " << endl;
  cout << public_port << " => " << FLAGS_private_port << endl;
  cout << "Lifetime: " << lifetime << endl;
  return 0;
}
