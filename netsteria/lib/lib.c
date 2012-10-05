
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>

#include "lib.h"


const char *uip_homedir;

static int gotkey;
static pthread_key_t key;


struct perthread *uip_perthread(void)
{
	assert(gotkey);

	struct perthread *pt = pthread_getspecific(key);
	if (pt == NULL) {
		pt = calloc(1, sizeof(*pt));
		if (pt == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		pt->hubsock = -1;

		if (pthread_setspecific(key, pt))
			return NULL;
	}
	return pt;
}

static void destructor(void *perthread)
{
	struct perthread *pt = perthread;

	if (pt->hubsock >= 0)
		close(pt->hubsock);
	free(pt);
}

int uip_init(void)
{
	// Find the user's home directory
	if (uip_homedir == NULL) {
		uip_homedir = getenv("HOME");
		if (uip_homedir == NULL) {
			errno = ENOENT;
			return -1;
		}
	}

	// Allocate our pthread key if we haven't already
	if (!gotkey) {
		if (pthread_key_create(&key, destructor) != 0)
			return -1;
		gotkey = 1;
	}

	if (uiplocom_init() < 0)
		return -1;
	if (uiparchive_init() < 0)
		return -1;

	return 0;
}

