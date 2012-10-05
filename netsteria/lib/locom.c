
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "lib.h"
#include "locom.h"


// Obtain the current thread's socket connecting to the local UIP hub,
// creating a new local hub connection if necessary.
static int gethubsock()
{
	// The hubsock lives in our per-thread structure.
	struct perthread *pt = uip_perthread();
	if (pt == NULL)
		return -1;

	// If already connected, we're done.
	if (pt->hubsock >= 0)
		return pt->hubsock;

	// Form the UIP hub's socket name
	char name[strlen(uip_homedir)+32];
	sprintf(name, "%s/.uip/locsock", uip_homedir);

	// Make sure it isn't too long for sockaddr_un
	struct sockaddr_un sun;
	sun.sun_family = AF_UNIX;
	if (strlen(name)+1 > sizeof(sun.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(sun.sun_path, name);

	// Make a UNIX domain stream socket and connect to the UIP hub
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;
	if (connect(sock, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
		// XX start the UIP hub automatically
		close(sock);
		return -1;
	}

#ifdef USE_PASSCRED
	// Enable credential passing
	int passcred = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED,
			&passcred, sizeof(int)) < 0) {
		close(sock);
		return -1;
	}

	// Authenticate ourselves to the local server.
	uint8_t cbuf[CMSG_SPACE(sizeof(struct ucred))];

	struct msghdr mh;
	memset(&mh, 0, sizeof(mh));
	mh.msg_control = cbuf;
	mh.msg_controllen = sizeof(cbuf);

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&mh);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_CREDENTIALS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct ucred));

	struct ucred *uc = (struct ucred*)CMSG_DATA(cmsg);
	uc->pid = getpid();
	uc->uid = getuid();
	uc->gid = getgid();

	mh.msg_controllen = cmsg->cmsg_len;

	if (sendmsg(sock, &mh, 0) < 0) {
		close(sock);
		return -1;
	}

	// Re-disable credential passing
	passcred = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED,
			&passcred, sizeof(int)) < 0) {
		close(sock);
		return -1;
	}

#endif	// USE_PASSCRED

	return (pt->hubsock = sock);
}

// Close the current thread's hub connection socket,
// e.g., after detecting a protocol error of some kind.
static void closehubsock()
{
	struct perthread *pt = uip_perthread();
	if (pt == NULL)
		return;

	if (pt->hubsock >= 0) {
		close(pt->hubsock);
		pt->hubsock = -1;
	}
}

int uiplocom_call(const void *request, size_t reqsize,
		void *replybuf, size_t replymax)
{
	const struct lochdr *requesthdr = request;
	assert(requesthdr->size == reqsize);
	assert(reqsize >= sizeof(struct lochdr));

	struct lochdr *replyhdr = replybuf;
	assert(replymax >= sizeof(struct lochdr));
	assert(replymax <= LOCMSG_MAX);

	// Find our thread's UIP hub connection socket
	int sock = gethubsock();
	if (sock < 0)
		return -1;

	// Send the request
	do {
		ssize_t rc = write(sock, request, reqsize);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		request += rc;
		reqsize -= rc;
	} while (reqsize > 0);

	// Await the response.
	size_t ract = 0;
	do {
		ssize_t rc = read(sock, replybuf+ract, replymax-ract);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		ract += rc;

		// Wait until at least the reply header's come in
		if (ract < sizeof(struct lochdr))
			continue;

		// Verify that we only receive as much data as expected,
		// and that the reply will actually fit in the buffer.
		if (ract > replyhdr->size || replyhdr->size > replymax) {
			errno = EPROTO;
			closehubsock();
			return -1;
		}

		// Wait for the whole reply to come in
		if (ract == replyhdr->size)
			break;

	} while (1);

	// On an error response, just return the error.
	if (replyhdr->code != 0) {
		errno = replyhdr->code;
		return -1;
	}

	// Return the size of the received response on success.
	return ract;
}

// Initialize the connection to the local UIP hub,
// at least on the current thread,
// and verify that the hub is responding to requests.
// (Other threads will establish connections lazily.)
int uiplocom_init()
{
	struct lochdr m;
	m.size = sizeof(m);
	m.code = LOCMSG_NULL;
	if (uiplocom_call(&m, sizeof(m), &m, sizeof(m)) < 0)
		return -1;

	return 0;
}

