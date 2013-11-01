// public-ip.c
// Author: Allen Porter <allen@thebends.org>
//
// Simple tool that makes a NAT PMP request to your gateway, and returns the
// public IP of the router.

#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sysexits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include "util.h"

using namespace std;

int main(int argc, char* argv[]) {
  in_addr public_ip;
  uint32_t uptime;
  if (!pmp::GetGatewayInfo(&public_ip, &uptime)) {
    errx(1, "pmp::GetGatewayInfo failed");
  }

  cout << "Gateway public ip: " << inet_ntoa(public_ip) << endl;
  cout << "Gateway uptime: " << uptime << endl;
  return 0;
}
