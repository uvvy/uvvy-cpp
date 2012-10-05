
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <openssl/rand.h>

#include <uip/archive.h>

#include "hub.h"
#include "peer.h"
#include "byteorder.h"


#define GOSSIP_RETRY	(1*1000000)	// Initial retry delay - 1 second
#define GOSSIP_PERIOD	(10*1000000)	// Ping peers every 10 seconds


#define GOSSIP_MAGIC		0x6741	// Magic value for requests ('gA')

// Message codes
#define GOSSIP_STATUSREQUEST	1	// Supply and request current status
#define GOSSIP_STATUSREPLY	2	// Supply and acknowledge status
#define GOSSIP_INFOREQUEST	3	// Request info about chunk(s)
#define GOSSIP_INFOREPLY	4	// Supply requested chunk info
#define GOSSIP_DATAREQUEST	5	// Request data from particular chunk
#define GOSSIP_DATAREPLY	6	// Supply requested chunk data

// Max size of a data block transferred in one gossip packet
#define GOSSIP_BLOCKMAX		1024

// Max # of chunks for which we can request info at once:
// 1024 (data block size) / 128 (info block size) = 8
#define GOSSIP_INFOMAX		8	// Max # chunks to get info on at once

// Max # of blocks of a chunk to request at one time
#define GOSSIP_MAXBLOCKS	8


struct gossiphdr {
	uint16_t magic;
	uint16_t code;
};

struct gossipstatus {
	struct gossiphdr hdr;
	uint32_t reserved;
	uint64_t curid;
};

struct gossipinforequest {
	struct gossiphdr hdr;
	uint8_t nchunks;		// # of chunk ids to get info on: <= 8
	uint8_t reserved1;
	uint16_t reserved2;
	uint64_t chunkid[GOSSIP_INFOMAX];	// chunkids to get info on
};

struct gossipinforeply {
	struct gossiphdr hdr;
	uint8_t nchunks;		// # of chunk info blocks returned
	uint8_t reserved1;
	uint16_t reserved2;
	uipchunkinfo info[GOSSIP_INFOMAX];
};

struct gossipdatarequest {
	struct gossiphdr hdr;
	uint16_t blkstart;		// 1024-byte block to start with
	uint16_t blkcount;		// number of consecutive blocks to send
	uipchunkid chunkid;		// chunk number to read
};

struct gossipdatareply {
	struct gossiphdr hdr;
	uint16_t blkno;			// block number of this block
	uint16_t reserved;
	uipchunkid chunkid;		// chunk number block is taken from
	char data[GOSSIP_BLOCKMAX];	// chunk data block
};

union gossipmsg {
	struct gossiphdr hdr;
	struct gossipstatus status;
	struct gossipinforequest inforequest;
	struct gossipinforeply inforeply;
	struct gossipdatarequest datarequest;
	struct gossipdatareply datareply;
};

static void gossiptime(void *cbdata);

static Timer gossiptimer(gossiptime, NULL);


struct GossipPeer
{
	Peer *peer;

	// Peer name and address
	const char *hostname;
	struct sockaddr_in sin;

	// Max rate at which to accept data from this peer
	float bytespersec;

	// Peer's chunk ID below which we have copies of all of peer's data
	uipchunkid readid;

	// Limit of chunk IDs known to exist on this peer
	uipchunkid limid;

	// Timer for sending status pings
	Timer pingtimer;

	// Current delay for block-and-backoff retry
	unsigned retrydelay;

	// Chunk transfer state
	int grabstate;	// 0=wait, 1=get info, 2=get data
	uint8_t grabqpos;	// current grab queue position
	uint8_t grabqmax;	// number of valid chunk infos in grab queue
	Timer grabtimer;
	char *grabgot;		// one 1-byte 'received' flag per block
	void *grabbuf;		// chunk receive buffer
	uipchunkinfo grabinfo[GOSSIP_INFOMAX];	// grab queue


	GossipPeer(Peer *p);
	~GossipPeer();
	void sendtopeer(const void *msg, size_t size);
	void sendstatus();
	int grabchunk(uipchunkinfo *ci);
	void grabstuff();
	void gotstatus(const gossipstatus *msg, size_t sz);
	void inforequest(const gossipinforequest *msg, size_t sz);
	void inforeply(const gossipinforeply *msg, size_t sz);
	void datarequest(const gossipdatarequest *msg, size_t sz);
	void datareply(const gossipdatareply *msg, size_t sz);
	void handle(const void *msg, size_t sz);
};



static void pingcallback(void *data);
static void grabcallback(void *data);

GossipPeer::GossipPeer(Peer *peer)
:	pingtimer(pingcallback, this),
	grabtimer(grabcallback, this)
{
	this->peer = peer;
	peer->gosspeer = this;

	this->readid = peer->readid;
	this->limid = readid;	// until host tells us what its ID limit is

	this->retrydelay = GOSSIP_RETRY;

	this->grabstate = 0;
	this->grabqpos = 0;	// queue empty
	this->grabqmax = 0;
	this->grabgot = NULL;
	this->grabbuf = NULL;

	// start and keep the ping timer running
	sendstatus();

	if (bytespersec > 0.0)
		grabtimer.setafter(1);	// grab first block immediately
}

GossipPeer::~GossipPeer()
{
	assert(peer != NULL);
	assert(peer->gosspeer == this);

	peer->gosspeer = NULL;
	peer = NULL;
}

void GossipPeer::sendtopeer(const void *msg, size_t size)
{
	flow_transmit(peer, msg, size);
}

void GossipPeer::sendstatus()
{
	debug(1, "Gossip: send status to '%s': chunk id %llu",
		peer->hostname, (unsigned long long)nextchunkid);

	// Send a (not necessarily solicited) status reply to this peer
	struct gossipstatus msg;
	msg.hdr.magic = htob16(GOSSIP_MAGIC);
	msg.hdr.code = htob16(GOSSIP_STATUSREPLY);
	msg.reserved = 0;
	msg.curid = htob64(nextchunkid);
	sendtopeer(&msg, sizeof(msg));

	// Re-arm the ping timer with a randomly jittered timeout
	// between GOSSIP_PERIOD and GOSSIP_PERIOD * 2.
	if (pingtimer.armed())
		pingtimer.disarm();
	uint32_t delay;
	pseudorand(&delay, sizeof(delay));
	delay = (delay % GOSSIP_PERIOD) + GOSSIP_PERIOD;
	pingtimer.setafter(delay);
}

static void pingcallback(void *cbdata)
{
	GossipPeer *p = (GossipPeer*)cbdata;
	p->sendstatus();
}

int GossipPeer::grabchunk(uipchunkinfo *ci)
{
	size_t size = btoh32(ci->size);
	unsigned nblocks = (size+GOSSIP_BLOCKMAX-1) / GOSSIP_BLOCKMAX;

	// Allocate the grab buffers if needed
	if (grabgot == NULL) {

		// Make sure we don't already have this chunk
		if (archive_lookuphash(ci->hash, NULL) == 0)
			return 1;	// already have it - trivial success

		// Make sure it has a reasonable size -
		// if not, just skip it entirely.
		if (size < UIP_MINCHUNK || size > UIP_MAXCHUNK) {
			unsigned long long id = btoh64(ci->chunkid);
			warn("block %llu on peer %s has invalid size %d",
				id, hostname, size);
			return 1;
		}

		// Allocate our buffers
		grabgot = (char*)calloc(1, nblocks);
		grabbuf = malloc(size);
		if (grabgot == NULL || grabbuf == NULL)
			fatal("out of memory");

		debug(1, "Gossip: grabbing chunk %llu key %016llx from %s",
			btoh64(ci->chunkid),
			btoh64(*(uint64_t*)ci->hash),
			hostname);
	}

	// Request missing chunk data
	unsigned i, j;
	for (i = 0; ; i++) {
		if (i == nblocks) {

			// Write it to our own archive,
			// verifying it against ci->hash in the process.
			int rc = 1;
			uipchunkinfo lci;
			if (archive_writechunk(grabbuf, size, ci->hash,
						&lci) < 0) {
				debug(1, "Gossip: error writing chunk %llu: %s",
					(unsigned long long)btoh64(ci->chunkid),
					strerror(errno));
				rc = -1;
			}

			debug(1, "Gossip: received chunk %llu -> "
					"local ID %llu, key %016llx size %d",
				(unsigned long long)btoh64(ci->chunkid),
				(unsigned long long)btoh64(lci.chunkid),
				(long long)btoh64(*(uint64_t*)ci->hash), size);

			// Free the chunk buffer
			// and return either success or "hard" failure.
			free(grabgot); grabgot = NULL;
			free(grabbuf); grabbuf = NULL;
			return rc;
		}
		if (grabgot[i] == 0)
			break;
	}
//	for (j = i+1; j < nblocks; j++)
//		if (grabgot[i] != 0)
//			break;

	debug(1, "Gossip: request chunk ID %llu blocks %d-%d from '%s'",
		(unsigned long long)btoh64(ci->chunkid), i, i, peer->hostname);

	// Send a request for the missing block
	gossipdatarequest msg;
	msg.hdr.magic = htob16(GOSSIP_MAGIC);
	msg.hdr.code = htob16(GOSSIP_DATAREQUEST);
	msg.blkstart = htob16(i);
	msg.blkcount = htob16(1);	// XX j-i
	msg.chunkid = ci->chunkid;
	sendtopeer(&msg, sizeof(msg));

	grabtimer.setafter(retrydelay);
	return 0;
}

void GossipPeer::grabstuff()
{
	if (grabtimer.armed())
		grabtimer.disarm();

	// If we're all caught up with the peer's archive,
	// just leave the timer disarmed and do nothing.
	if (readid >= limid)
		return;

	// Grab any chunks already in the queue
	while (grabqpos < grabqmax) {

		uipchunkinfo *ci = &grabinfo[grabqpos];
		int rc = grabchunk(ci);
		if (rc == 0)
			return;		// still working - not complete yet

		// Bump our readid if we just read the next sequential chunk
		if (rc > 0) {
			uipchunkid cid = btoh64(ci->chunkid);
			if (cid == readid)
				readid++;
		}

		// Move on to the next chunk in the grab queue
		// regardless of whether the current one succeeded or failed.
		// If it failed, we'll come back to it sooner or later.
		grabqpos++;
	}

	// Request chunk info to refill the grab queue
	assert (grabqpos >= grabqmax);

	gossipinforequest req;
	req.hdr.magic = htob16(GOSSIP_MAGIC);
	req.hdr.code = htob16(GOSSIP_INFOREQUEST);
	req.reserved1 = 0;
	req.reserved2 = 0;

	// Choose chunk ids on which to request info.
	// If there are fewer than GOSSIP_INFOMAX new chunks,
	// this task is trivial - just request them all.
	uipchunkid newchunks = limid - readid;
	if (newchunks <= GOSSIP_INFOMAX) {

		// Request chunks sequentially
		req.nchunks = newchunks;
		for (unsigned i = 0; i < newchunks; i++)
			req.chunkid[i] = htob64(readid + i);
		debug(1, "Gossip: requesting %d sequential chunks "
			"starting with ID %llu",
			newchunks, (unsigned long long)readid);
	} else {

		// Too many new chunks to request them all at once.
		// Pick GOSSIP_INFOMAX/2 random chunk ids,
		// followed by GOSSIP_INFOMAX/2 sequential ids.
		// inforeply() will prioritize the random chunks.
		req.nchunks = GOSSIP_INFOMAX;
		for (int i = 0; i < GOSSIP_INFOMAX/2; i++) {
			uint64_t sel;
			pseudorand(&sel, sizeof(sel));
			sel %= newchunks;
			req.chunkid[i] = htob64(readid + sel);
		}
		for (int i = 0; i < GOSSIP_INFOMAX/2; i++) {
			req.chunkid[GOSSIP_INFOMAX/2 + i] =
					htob64(readid + i);
		}
		debug(1, "Gossip: requesting random chunk IDs %llu, %llu, ...",
			(unsigned long long)btoh64(req.chunkid[0]),
			(unsigned long long)btoh64(req.chunkid[1]));
	}

	// Send off the chunk info request
	size_t sz = offsetof(gossipinforequest, chunkid) +
			sizeof(uipchunkid) * req.nchunks;
	sendtopeer(&req, sz);

	grabtimer.setafter(retrydelay);
}

static void grabcallback(void *cbdata)
{
	GossipPeer *p = (GossipPeer*)cbdata;

	// Timeout occurred - increase retry delay
	p->retrydelay = (p->retrydelay * 3) / 2;
	if (p->retrydelay > GOSSIP_PERIOD)
		p->retrydelay = GOSSIP_PERIOD;

	// Try again whatever we were doing
	p->grabstuff();
}

// Handle a received status request or reply message
void GossipPeer::gotstatus(const gossipstatus *msg, size_t sz)
{
	if (sz < sizeof(gossipstatus)) {
		debug(1, "bad gossip status message from %s", hostname);
		return;
	}

	// Accept the new chunk id limit
	uipchunkid cid = btoh64(msg->curid);
	if (cid > limid) {

		// More new chunks available from this peer!
		debug(1, "status from %s: chunks %llu-%llu now available! "
			"(currently at %llu)",
			hostname, (unsigned long long)limid,
			(unsigned long long)cid-1,
			(unsigned long long)readid);
		limid = cid;

	} else if (cid < limid) {

		// Peer's archive was probably reset: rescan it from the start
		warn("gossip peer %s's chunk id limit went backwards! "
			"(archive reset?)\n", hostname);
		limid = readid = 0;
	}

	if (limid > readid)
		grabstuff();	// XX should be rate-controlled
}

void GossipPeer::inforequest(const gossipinforequest *msg, size_t sz)
{
	// Check the request message size
	if (sz < offsetof(gossipinforequest, chunkid)) {
		badmsg:
		debug(1, "Gossip: bad info request from %s", hostname);
		return;
	}
	unsigned nchunks = msg->nchunks;
	if (nchunks == 0 || nchunks > GOSSIP_INFOMAX)
		goto badmsg;
	if (sz < offsetof(gossipinforequest, chunkid) +
			nchunks * sizeof(uipchunkid))
		goto badmsg;

	// Build the reply message.
	gossipinforeply rpl;
	rpl.hdr.magic = htob16(GOSSIP_MAGIC);
	rpl.hdr.code = htob16(GOSSIP_INFOREPLY);
	rpl.reserved1 = 0;
	rpl.reserved2 = 0;

	// Lookup the specified chunk IDs.
	unsigned outchunks = 0;
	for (unsigned i = 0; i < nchunks; i++) {
		uipchunkid cid = btoh64(msg->chunkid[i]);
		if (archive_lookupid(cid, &rpl.info[outchunks]) < 0) {
			debug(1, "peer %s requested invalid chunk ID %llu",
					hostname, cid);
			continue; // just skip any invalid chunk IDs
		}
		outchunks++;
	}
	if (outchunks == 0)
		return;
	rpl.nchunks = outchunks;

	// Send the reply
	sendtopeer(&rpl, offsetof(gossipinforequest, chunkid) +
				nchunks * sizeof(uipchunkinfo));
}

// Handle a received chunk info reply message
void GossipPeer::inforeply(const gossipinforeply *msg, size_t sz)
{
	// Just throw it away if we have a nonempty grab queue
	if (grabqpos < grabqmax) {
		debug(1, "redundant gossip info reply received from %s "
			"while grab queue is non-empty", hostname);
		return;
	}

	// Check the info reply message size
	if (sz < offsetof(gossipinforequest, chunkid)) {
		debug(1, "Gossip: info reply from %s too short", hostname);
		badmsg:
		return;
	}
	unsigned nchunks = msg->nchunks;
	if (nchunks == 0 || nchunks > GOSSIP_INFOMAX) {
		debug(1, "Gossip: info reply contains %d chunks??", nchunks);
		goto badmsg;
	}
	size_t requiredsize = offsetof(gossipinforequest, chunkid) +
				sizeof(uipchunkid) * nchunks;
	if (sz < requiredsize) {
		debug(1, "Gossip: info reply from %s too short", hostname);
		goto badmsg;
	}

	// Copy the returned chunk info blocks into our queue
	memcpy(grabinfo, msg->info, sizeof(uipchunkinfo) * nchunks);
	grabqpos = 0;
	grabqmax = nchunks;

	debug(1, "Gossip: received info on %d chunks", nchunks);

	// Find the first chunk in the list that we don't already have,
	// incrementing our readid as we skip past sequential chunks.
	uipchunkid cid;
	do {
		const uipchunkinfo *ci = &msg->info[grabqpos];
		cid = btoh64(ci->chunkid);

		if (archive_lookuphash(ci->hash, NULL) < 0)
			break;	// don't already have it

		if (cid == readid)
			readid++;
		grabqpos++;
	} while (grabqpos < nchunks);

	// If we found a randomly-selected (non-sequential) chunk
	// that we don't already have,
	// then chop the GOSSIP_INFOMAX/2 sequential chunks off the list
	// and only grab the random chunks.
	if (grabqpos < GOSSIP_INFOMAX/2 && grabqmax > GOSSIP_INFOMAX/2 &&
			cid != readid)
		grabqmax = GOSSIP_INFOMAX/2;

	// Request a chunk from our new queue, either now or later
	retrydelay = GOSSIP_RETRY;
	grabstuff();
}

void GossipPeer::datarequest(const gossipdatarequest *msg, size_t sz)
{
	if (sz < sizeof(*msg)) {
		debug(1, "Gossip: data request from %s too short", hostname);
		badmsg:
		return;
	}

	uipchunkid cid = btoh64(msg->chunkid);
	unsigned blkstart = btoh16(msg->blkstart);
	unsigned blkcount = btoh16(msg->blkcount);
	blkcount = 1;	// XXX

	// Lookup the requested chunk in our archive
	uipchunkinfo info;
	if (archive_lookupid(cid, &info) < 0) {
		debug(1, "data request from %s for unavailable chunk",
				hostname);
		return; // XX return error reply?
	}

	size_t size = btoh32(info.size);
	unsigned nblocks = (size+GOSSIP_BLOCKMAX-1) / GOSSIP_BLOCKMAX;
	if (blkstart + blkcount > nblocks) {
		debug(1, "Gossip: request for invalid blocks %d-%d "
			"in chunk %llu which has only %d blocks",
			blkstart, blkstart+blkcount-1,
			(unsigned long long)cid, nblocks);
		goto badmsg;
	}

	for (unsigned i = 0; i < blkcount; i++) {

		// Compute the size of this block
		unsigned blkno = blkstart + i;
		unsigned blkofs = blkno*GOSSIP_BLOCKMAX;
		unsigned blksize = size - blkofs;
		if (blksize > GOSSIP_BLOCKMAX)
			blksize = GOSSIP_BLOCKMAX;

		// Build and send the reply
		gossipdatareply rpl;
		rpl.hdr.magic = htob16(GOSSIP_MAGIC);
		rpl.hdr.code = htob16(GOSSIP_DATAREPLY);
		rpl.blkno = htob16(blkno);
		rpl.reserved = 0;
		rpl.chunkid = htob64(cid);
		if (archive_readchunkpart(&info, rpl.data, blkofs, blksize)
				< 0) {
			debug(1, "data request from %s for unavailable "
					"chunk data", hostname);
			return;	// XX error reply?
		}
		sendtopeer(&rpl, offsetof(gossipdatareply, data) + blksize);
	}
}

void GossipPeer::datareply(const gossipdatareply *msg, size_t sz)
{
	// Make sure we're actually expecting chunk data
	if (grabgot == NULL) {
		debug(1, "unwanted chunk data received from %s", hostname);
		return;
	}

	assert(grabqpos < grabqmax);
	const uipchunkinfo *ci = &grabinfo[grabqpos];
	size_t size = btoh32(ci->size);
	unsigned nblocks = (size+GOSSIP_BLOCKMAX-1) / GOSSIP_BLOCKMAX;

	// Check message size and chunk ID
	if (sz < offsetof(gossipdatareply, data)) {
		badmsg:
		debug(1, "bad chunk data reply from %s", hostname);
		return;
	}
	if (msg->chunkid != ci->chunkid) {
		debug(1, "data received from %s for wrong chunk", hostname);
		return;
	}

	// Make sure it's a block we actually need
	uint16_t blkno = btoh16(msg->blkno);
	if (blkno >= nblocks)
		goto badmsg;
	if (grabgot[blkno]) {
		debug(1, "duplicate chunk data received from %s", hostname);
		return;
	}

	// Determine the proper size of this block
	unsigned blksize = size - blkno*GOSSIP_BLOCKMAX;
	if (blksize > GOSSIP_BLOCKMAX)
		blksize = GOSSIP_BLOCKMAX;
	if (sz < offsetof(gossipdatareply, data) + blksize)
		goto badmsg;

	// Save the block in our chunk buffer
	memcpy((char*)grabbuf + blkno*GOSSIP_BLOCKMAX, msg->data, blksize);
	grabgot[blkno] = 1;

	// Move on to the next chunk, or whatever
	retrydelay = GOSSIP_RETRY;
	grabstuff();
}

void GossipPeer::handle(const void *msg, size_t sz)
{
	gossipmsg *m = (gossipmsg*)msg;
	if (m->hdr.magic != htob16(GOSSIP_MAGIC)) {
		debug(1, "bad magic %04x in received gossip message",
			btoh16(m->hdr.magic));
		return;
	}

	uint16_t code = btoh16(m->hdr.code);
	switch (code) {
	case GOSSIP_STATUSREQUEST:
		sendstatus();
		// fall through...
	case GOSSIP_STATUSREPLY:
		gotstatus(&m->status, sz);
		break;
	case GOSSIP_INFOREQUEST:
		inforequest(&m->inforequest, sz);
		break;
	case GOSSIP_INFOREPLY:
		inforeply(&m->inforeply, sz);
		break;
	case GOSSIP_DATAREQUEST:
		datarequest(&m->datarequest, sz);
		break;
	case GOSSIP_DATAREPLY:
		datareply(&m->datareply, sz);
		break;
	default:
		debug(1, "unknown gossip code %d", code);
		break;
	}
}

void gossip_receive(Peer *p, const void *msg, size_t sz)
{
	p->gosspeer->handle(msg, sz);
}

static void gossiptime(void *cbdata)
{
	// Re-arm the timer immediately
	gossiptimer.setafter(GOSSIP_PERIOD);

	for (Peer *p = peers; p != NULL; p = p->next)
		p->gosspeer->sendstatus();
}

void gossip_init()
{
}

void gossip_initpeer(Peer *p)
{
	p->gosspeer = new GossipPeer(p);
}

void gossip_delpeer(Peer *p)
{
	assert(p->gosspeer != NULL);
	delete p->gosspeer;
}

