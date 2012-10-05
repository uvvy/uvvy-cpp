// Local communication - servicing the requests of local apps
// via messages on our UNIX domain socket.

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "hub.h"
#include "locom.h"


static int locsock = -1;

struct LocalClient {
	LocalClient *next;
	int fd;
	uid_t uid;

	uint8_t *buf;
	size_t bufsize;
	size_t bufofs;
	size_t replysize;

	unsigned	authing : 1,
			reading : 1,
			writing : 1;

	LocalClient(int fd, uid_t uid);
	~LocalClient();
	void check();
	ssize_t writechunk(size_t msgsize);
	size_t handle(size_t msgsize);
};
static LocalClient *clients;

#define INIT_BUFSIZE	1024


LocalClient::LocalClient(int fd, uid_t uid)
{
printf("new local client fd %d\n", fd);
	this->fd = fd;
	this->uid = uid;

	this->buf = new uint8_t[INIT_BUFSIZE];
	this->bufsize = INIT_BUFSIZE;
	this->bufofs = 0;

	this->authing = 1;
	this->reading = 0;
	this->writing = 0;

	setnonblock(fd);
	watchfd(fd, 0, 1);		// Watch for reading

	this->next = clients;
	clients = this;
}

void LocalClient::check()
{
	ssize_t rc;

	if (authing) {

		// We're waiting for client authentication.
#ifdef USE_PASSCRED
		struct iovec iov[1];
		iov[0].iov_base = buf;
		iov[0].iov_len = bufsize;

		uint8_t cbuf[CMSG_SPACE(sizeof(struct ucred))];

		struct msghdr mh;
		memset(&mh, 0, sizeof(mh));
		mh.msg_iov = iov;
		mh.msg_iovlen = 1;
		mh.msg_control = cbuf;
		mh.msg_controllen = sizeof(cbuf);

		rc = recvmsg(fd, &mh, 0);
		if (rc < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK &&
					errno != EINTR) {
				warn("error authenticating client: %s",
					strerror(errno));
				delete this;
			}
			return;
		}
		bufofs += rc;
		assert(bufofs <= bufsize);

		// Extract the credentials
		cmsghdr *cmsg = CMSG_FIRSTHDR(&mh);
		if (cmsg == NULL ||
				cmsg->cmsg_level != SOL_SOCKET ||
				cmsg->cmsg_type != SCM_CREDENTIALS) {
			if (bufofs > 0) {
				// Client sent data before credentials!
				warn("local client failed to pass credentials");
				delete this;
			}
			return;
		}
		struct ucred *uc = (ucred*)CMSG_DATA(cmsg);

		// Check received credentials
		uid_t uid = uc->uid;
		if (!multiuser && uid != getuid()) {
			debug(1, "local connection attempt from wrong user - "
					"rejecting");
			delete this;
			return;
		}

		// No more credentials needed now
		int passcred = 0;
		(void)setsockopt(fd, SOL_SOCKET, SO_PASSCRED,
				&passcred, sizeof(int));
#endif

		authing = 0;
		reading = 1;
		goto donereading;
	}

	if (reading) {

		// We're waiting for a message to come in.  Read it.
		assert(bufsize > bufofs);
		rc = read(fd, buf+bufofs, bufsize-bufofs);
		if (rc < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK &&
					errno != EINTR) {
				debug(1, "error reading from client: %s",
					strerror(errno));
				closeit:
				delete this;
			}
			return;
		}
		if (rc == 0) {	// End of stream: client closed his end
			debug(1, "client %d closed connection", fd);
			goto closeit;
		}
		bufofs += rc;
		assert(bufofs <= bufsize);
		donereading:

		// Wait for and check the message size
		if (bufofs < sizeof(lochdr))
			return;
		locmsg *msg = (locmsg*)buf;
		size_t msgsize = msg->hdr.size;
		if (msgsize < sizeof(struct lochdr) || msgsize > LOCMSG_MAX) {
			// Invalid message size - close the connection.
			debug(1, "invalid message size from local client");
			goto closeit;
		}

		// Grow our input buffer if necessary
		if (msgsize > bufsize) {
			uint8_t *newbuf = new uint8_t[msgsize];
			memcpy(newbuf, buf, bufofs);
			delete [] buf;
			buf = newbuf;
			bufsize = msgsize;
		}

		// Wait until the whole message comes in
		if (bufofs < msgsize)
			return;

		// It's a protocol error if more than the msgsize comes in,
		// because the client should be waiting for our response.
		if (bufofs > msgsize) {
			debug(1, "local client sent too much data");
			goto closeit;
		}

		reading = 0;
		watchfd(fd, 0, 0);

		// Process the request and send a reply.
		// XX want to be able to delay reply?
		replysize = handle(msgsize);
		assert(replysize >= sizeof(struct lochdr));
		assert(replysize <= bufsize);

		// Switch to writing mode to send the response
		writing = 1;
		bufofs = 0;
		watchfd(fd, 1, 1);
	}

	if (writing) {

		// We're sending a response.  Write it out.
		rc = write(fd, buf+bufofs, replysize-bufofs);
		if (rc < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK &&
					errno != EINTR) {
				warn("error reading from client: %s",
					strerror(errno));
				delete this;
			}
			return;
		}
		if (rc == 0) {
			debug(1, "client %d unexpectedly closed connection", fd);
			goto closeit;
		}
		bufofs += rc;
		assert(bufofs <= replysize);

		// Wait until we flush the whole reply message
		if (bufofs < replysize)
			return;

		// Switch back to reading mode to get the next message
		writing = 0;
		watchfd(fd, 1, 0);
		reading = 1;
		bufofs = 0;
		watchfd(fd, 0, 1);
	}
}

// Handle a chunk write request from a local client.
ssize_t LocalClient::writechunk(size_t msgsize)
{
	union locmsg *msg = (locmsg*)buf;

	// Validate the chunk size
	size_t chunksize = msgsize - sizeof(lochdr);
	if (chunksize < UIP_MINCHUNK || chunksize > UIP_MAXCHUNK) {
		debug(1, "Locom: received request to write chunk "
			"with invalid size %d", chunksize);
		errno = EINVAL;
		return -1;
	}

	// Write the chunk
	uipchunkinfo info;
	if (archive_writechunk(msg->writechunkrequest.data, chunksize,
				NULL, &info) < 0)
		return -1;

	// Return the resulting chunk info to the local client
	msg->writechunkreply.info = info;
	return sizeof(locwritechunkreply);
}

// Process a complete message received from a client.
// Returns the size of the reply message to return.
size_t LocalClient::handle(size_t msgsize)
{
	union locmsg *msg = (locmsg*)buf;
	ssize_t rc = 0;
	switch (msg->hdr.code) {

	case LOCMSG_NULL:
		// Do nothing - just return a minimal success response.
		rc = sizeof(lochdr);
		break;

	case LOCMSG_WRITECHUNK:
		rc = writechunk(msgsize);
		break;
	default:
		errno = EINVAL;
		rc = -1;
		break;
	}

	if (rc >= 0) {
		assert(rc >= (ssize_t)sizeof(lochdr));
		msg->hdr.size = rc;
		msg->hdr.code = 0;
		return rc;
	} else {
		msg->hdr.size = sizeof(locmsg);
		msg->hdr.code = errno;
		return sizeof(locmsg);
		// XX pass back a text error message too?
	}
}

LocalClient::~LocalClient()
{
	LocalClient *c, **cp;
	for (cp = &clients; (c = *cp) != this; cp = &c->next)
		assert(c != NULL);
	*cp = next;

	assert(fd >= 0);
	watchfd(fd, 0, 0);
	watchfd(fd, 1, 0);
	close(fd);

	assert(buf != NULL);
	delete buf;
}

// Check for and accept new client connections on our locsock
static void checknew()
{
	int sock = accept(locsock, NULL, NULL);
	if (sock < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			warn("error accepting new client connection: %s",
				strerror(errno));
		return;
	}

	debug(1, "accepted connection %d", sock);

	uid_t peeruid = getuid();
#ifdef HAVE_GETPEEREID
	if (getpeereid(sock, &peeruid, NULL) < 0) {
		warn("error identifying local client via getpeereid(): %s - "
			"rejecting connection", strerror(errno));
		close(sock);
		return;
	}
	if (!multiuser && peeruid != getuid()) {
		debug(1, "local connection attempt from wrong user - "
				"rejecting");
		close(sock);
		return;
	}
#endif

#ifdef USE_PASSCRED
	int passcred = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED,
			&passcred, sizeof(int)) < 0) {
		debug(1, "error enabling credentials receiving (SO_PASSCRED)");
		close(sock);
		return;
	}
#endif

	(void)new LocalClient(sock, peeruid);
}

void locom_check(fd_set *readfds, fd_set *writefds)
{
	if (FD_ISSET(locsock, readfds)) {
		checknew();
	}

	for (LocalClient *cli = clients; cli != NULL; cli = cli->next) {
		int fd = cli->fd;
		if (FD_ISSET(fd, readfds) || FD_ISSET(fd, writefds))
			cli->check();
	}
}

void locom_init(const char *uipdir)
{
	assert(locsock < 0);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		fatal("can't create Unix domain socket: %s", strerror(errno));

	// Form the socket name
	char name[strlen(uipdir)+32];
	sprintf(name, "%s/locsock", uipdir);

	// Make sure it isn't too long for sockaddr_un
	struct sockaddr_un sun;
	sun.sun_family = AF_UNIX;
	if (strlen(name)+1 > sizeof(sun.sun_path))
		fatal("can't create Unix domain socket '%s': "
			"name too long for struct sockaddr_un",
			name);
	strcpy(sun.sun_path, name);

	// Remove any previous socket
	unlink(name);

	// Bind it into the file system.
	// Mask permissions appropriately if we're not a multiuser hub.
	// Some systems (e.g., BSD) ignore these permissions, though,
	// so we still need to authenticate clients via getpeereid.
	mode_t oldumask = umask(multiuser ? 0000 : 0077);
	if (bind(sock, (struct sockaddr*)&sun, sizeof(sun)) < 0)
		fatal("can't bind Unix domain socket to path '%': %s",
			name, strerror(errno));
	umask(oldumask);

	// Listen for connections from local clients
	if (listen(sock, SOMAXCONN) < 0)
		fatal("can't listen on Unix domain socket '%s': %s",
			name, strerror(errno));

	setnonblock(sock);
	watchfd(sock, 0, 1);

#ifdef USE_PASSCRED
	int passcred = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED,
			&passcred, sizeof(int)) < 0)
		fatal("error enabling receiving of credentials (SO_PASSCRED)");
#endif

	locsock = sock;
}

