// Main private header for the UIP access library
#ifndef LIB_LIB_H
#define LIB_LIB_H


// Per-thread data for UIP library
struct perthread {
	int hubsock;
};


// lib.c
const char *uip_homedir;

int uip_init(void);
struct perthread *uip_perthread(void);

// locom.c
int uiplocom_init();
int uiplocom_call(const void *request, size_t reqsize,
		void *replybuf, size_t replymax);

// archive.c
int uiparchive_init();


#endif	// LIB_LIB_H
