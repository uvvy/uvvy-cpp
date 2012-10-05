
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "hub.h"
#include "peer.h"


Peer *peers;


Peer::Peer(const char *hostname, const struct sockaddr_in *sin,
		float bytespersec, uint64_t readid)
{
	this->hostname = strdup(hostname);
	assert(this->hostname != NULL);
	this->sin = *sin;
	this->bytespersec = bytespersec;
	this->readid = readid;

	this->next = peers;
	peers = this;

	flow_initpeer(this);
	gossip_initpeer(this);
}

Peer::~Peer()
{
	flow_delpeer(this);
	gossip_delpeer(this);
}

void peer_init(const char *uipdir)
{
	char name[strlen(uipdir)+30];
	sprintf(name, "%s/peers", uipdir);
	FILE *fh = fopen(name, "r");
	if (fh == NULL)
		fatal("can't open '%s': %s", name, strerror(errno));

	while (!feof(fh)) {
		char hostname[50];
		unsigned port;
		float bytespersec;
		long long unsigned readid;
		int rc = fscanf(fh, "%49s%u%f%llu", hostname, &port,
				&bytespersec, &readid);
		if (rc <= 0 && feof(fh))
			break;
		if (rc < 3)
			fatal("format error in '%s'", name);

		debug(1, "Peer %s:%d - rate %f BPS, id %llu",
			hostname, port, bytespersec, readid);

		struct hostent *he = gethostbyname(hostname);
		if (he == NULL)
			fatal("can't find host '%s'", hostname);
		assert(he->h_addrtype == AF_INET);
		assert(he->h_length == sizeof(in_addr));

		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_addr = *(in_addr*)he->h_addr;
		sin.sin_port = htons(port);
		debug(1, "host '%s': address %s", hostname,
			inet_ntoa(sin.sin_addr));

		(void)new Peer(hostname, &sin, bytespersec, readid);
	}
}

