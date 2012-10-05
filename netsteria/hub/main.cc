
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <openssl/rand.h>

#include "hub.h"


// Options settable via command-line
int verbosity;
int multiuser;

static const char *progname;

static int maxfd;
static fd_set readfds, writefds;

static Timer *timers;


void debug(int requiredverbosity, const char *fmt, ...)
{
	if (verbosity < requiredverbosity)
		return;

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void warn(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void fatal(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "%s: fatal error: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(2);
}

void
usage() {
	debug(0, "usage: %s [options]", progname);
	debug(0, "  options:");
	debug(0, "    -v verbose");

	exit(1);
}

// Set a file descriptor to nonblocking mode
void setnonblock(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void watchfd(int fd, int writefd, int enable)
{
	assert(fd >= 0);

	if (maxfd == 0) {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
	}

	fd_set *fds = writefd ? &writefds : &readfds;

	if (enable) {
		// Add this fd to the appropriate fd_set
		FD_SET(fd, fds);
		if (maxfd <= fd)
			maxfd = fd+1;
	} else {
		FD_CLR(fd, fds);
	}
}

void pseudorand(void *buf, size_t sz)
{
	if (RAND_pseudo_bytes((unsigned char*)buf, sz) < 0)
		fatal("openssl failed to produce random bytes");
}

static inline uipdate tv2date(const struct timeval *tv)
{
	return (uipdate)tv->tv_sec * 1000000 + tv->tv_usec;
}

static inline void date2tv(uint64_t date, struct timeval *tv)
{
	tv->tv_sec = date / 1000000;
	tv->tv_usec = date % 1000000;
}

uipdate getdate()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv2date(&tv);
}

Timer::Timer(void (*callback)(void *data), void *cbdata)
{
	this->callback = callback;
	this->cbdata = cbdata;
	this->date = 0;
}

void Timer::setat(uipdate date)
{
	assert(date != 0);
	assert(this->date == 0);

	this->date = date;

	// Insert this timer at the proper position in the timer list
	Timer **tp = &timers, *t;
	for (tp = &timers; (t = *tp) != NULL && t->date < date; tp = &t->next);
	this->next = t;
	*tp = this;
}

void Timer::setafter(uipdate micros)
{
	setat(getdate() + micros);
}

void Timer::disarm()
{
	assert(this->date != 0);
	this->date = 0;

	Timer **tp = &timers, *t;
	for (tp = &timers; (t = *tp) != this; tp = &t->next)
		assert(t != NULL);
	*tp = this->next;
}


static void init(int argc, char **argv)
{
	progname = argv[0];

	// Determine the appropriate default UIP state directory
	// XXX different default if run as root?
	const char *homedir = getenv("HOME");
	if (homedir == NULL)
		fatal("can't determine home directory");
	char defaultdir[strlen(homedir)+32];
	sprintf(defaultdir, "%s/.uip", homedir);
	const char *uipdir = defaultdir;

	int c;
	while ((c = getopt(argc, argv, "d:hp:v")) >= 0) {
		switch(c) {
		case 'd':
			uipdir = optarg;
			break;
		case 'p':
			flowport = atoi(optarg);
			if (flowport <= 0 || flowport > 65535)
				fatal("Invalid port number '%s'", optarg);
			break;
		case 'v':
			verbosity++;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
		
	}

	// Make sure the UIP state directory exists
	if (mkdir(uipdir, 0700) < 0 && errno != EEXIST)
		fatal("can't create UIP state directory '%s': %s",
			uipdir, strerror(errno));

	// Initialize our various modules
	archive_init(uipdir);
	locom_init(uipdir);
	flow_init();
	gossip_init();
	peer_init(uipdir);
}

int
main(int argc, char *argv[])
{
	init(argc, argv);

	uipdate lastdate = getdate();
	while (1) {

		// If the system clock is ever set backwards,
		// subtract the difference from all our timer expriations
		// so that we don't get huge effective expirations.
		uipdate curdate = getdate();
		if (curdate < lastdate) {
			uipdate diff = lastdate - curdate;
			for (Timer *t = timers; t != NULL; t = t->next)
				t->date -= diff;
		}
		lastdate = curdate;

		// Fire any timers that have expired.
		while (timers != NULL && timers->date <= curdate) {
			Timer *t = timers;
			t->date = 0;
			timers = t->next;
			t->callback(t->cbdata);
		}

		// Determine how long to wait before the next event.
		struct timeval tv, *tvp;
		if (timers != NULL)
			date2tv(timers->date - curdate, tvp = &tv);
		else
			tvp = NULL;

		// Wait for I/O events or the next timer event.
		fd_set rfds = readfds, wfds = writefds;
		if (select(maxfd, &rfds, &wfds, NULL, tvp) < 0) {
			if (errno == EINTR)
				continue;
			fatal("error waiting in select(): %s",
				strerror(errno));
		}

		locom_check(&rfds, &wfds);
		flow_check(&rfds, &wfds);
	}
}

