#ifndef HUB_PEER_H
#define HUB_PEER_H

#include <sys/socket.h>
#include <netinet/in.h>

#include <uip/archive.h>


struct FlowPeer;
struct GossipPeer;

struct Peer {
	Peer *next;

	// Config info
	const char *hostname;
	sockaddr_in sin;
	float bytespersec;
	uipchunkid readid;

	// Module-private information
	FlowPeer *flowpeer;
	GossipPeer *gosspeer;

	Peer(const char *hostname, const struct sockaddr_in *sin,
		float bytespersec, uint64_t readid);
	~Peer();
};

extern Peer *peers;

void peer_init(const char *uipdir);


#endif	// HUB_PEER_H
