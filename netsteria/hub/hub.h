#ifndef HUB_HUB_H
#define HUB_HUB_H

#include <sys/types.h>
#include <sys/select.h>

#include <uip/archive.h>


#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))


// main.cc
typedef int64_t uipdate;	// XXX uip/uip.h?

struct Timer {
	Timer *next;
	void (*callback)(void *data);
	void *cbdata;
	uipdate date;

	Timer(void (*callback)(void *data), void *cbdata);
	inline int armed() { return date != 0; }
	void setat(uipdate date);
	void setafter(uipdate micros);
	void disarm();
};

extern int multiuser;
extern int verbosity;

void debug(int requiredverbosity, const char *fmt, ...);
void warn(const char *fmt, ...);
void fatal(const char *fmt, ...);
uipdate getdate();
void setnonblock(int fd);
void watchfd(int fd, int writefd, int enable);
void pseudorand(void *buf, size_t sz);

// archive.cc
struct uipchunkinfo;
extern uipchunkid nextchunkid;

void archive_init(const char *uipdir);
int archive_lookupid(uipchunkid id, uipchunkinfo *out_info);
int archive_lookuphash(const uint8_t *outerhash, uipchunkinfo *out_info);
int archive_readchunkpart(const uipchunkinfo *info, void *buf,
				size_t chunkofs, size_t bufsize);
int archive_readchunk(const uipchunkinfo *info, void *buf);
int archive_writechunk(const void *buf, size_t size, const uint8_t *hash,
			struct uipchunkinfo *out_info);

// locom.cc
void locom_init(const char *uipdir);
void locom_check(fd_set *readfds, fd_set *writefds);

// peer.cc
struct Peer;

void peer_init(const char *uipdir);

// flow.cc
extern int flowport;

void flow_init();
void flow_initpeer(Peer *p);
void flow_delpeer(Peer *p);
void flow_transmit(Peer *p, const void *msg, size_t size);
void flow_check(fd_set *readfds, fd_set *writefds);

// gossip.cc
void gossip_init();
void gossip_initpeer(Peer *p);
void gossip_delpeer(Peer *p);
void gossip_receive(Peer *p, const void *msg, size_t sz);


#endif	// HUB_HUB_H
