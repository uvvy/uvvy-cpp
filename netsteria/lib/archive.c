
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

#include <openssl/aes.h>

#include <uip/archive.h>

#include "lib.h"
#include "locom.h"
#include "sha2.h"
#include "byteorder.h"


static int archfd = -1;

// Memory-mapped file containing the archive chunk index
static int chunkidxfd = -1;
//static uipchunkinfo *chunkidx;
//static size_t chunkidxsize;	// Size of chunkidx as mapped in memory

// Memory-mapped file containing the archive hash index
static int hashidxfd = -1;
//static uipchunkinfo *hashidx;
//static size_t hashidxlen;		// always a power of two



int uiparchive_init()
{
	char name[strlen(uip_homedir)+32];

	// Open the main archive file read-only
	// (only the UIP hub writes to it).
	if (archfd < 0) {
		sprintf(name, "%s/.uip/archive", uip_homedir);
		archfd = open(name, O_RDONLY);
		if (archfd < 0)
			return -1;
	}

	// Open the archive chunk index
	if (chunkidxfd < 0) {
		sprintf(name, "%s/.uip/archindex", uip_homedir);
		chunkidxfd = open(name, O_RDONLY);
		if (chunkidxfd < 0)
			return -1;
	}

	// Open the archive hash index
	if (hashidxfd < 0) {
		sprintf(name, "%s/.uip/hashindex", uip_homedir);
		hashidxfd = open(name, O_RDONLY);
		if (hashidxfd < 0)
			return -1;
	}

	return 0;
}

// Encrypt or decrypt a data chunk in-place using AES-256-CTR encryption.
// Assumes the data is 128-bit aligned and a multiple of 128 bits in size.
static void cryptchunk(const void *inbuf, void *outbuf, size_t size,
			const uint8_t *key)
{
	assert(size <= UIP_MAXCHUNK);

	AES_KEY aeskey;
	AES_set_encrypt_key(key, 256, &aeskey);

	union {
		uint8_t b[16];
		uint32_t w[4];
		uint64_t l[2];
	} cblk, eblk;
	memcpy(&cblk, "UIPchunkCntr", 12);

	// Encrypt whole blocks
	size_t i;
	for (i = 0; i+16 <= size; i += 16) {
		cblk.w[3] = htob32(i >> 4);
		AES_encrypt(cblk.b, eblk.b, &aeskey);
		((uint64_t*)(outbuf+i))[0] =
				((const uint64_t*)(inbuf+i))[0] ^ eblk.l[0];
		((uint64_t*)(outbuf+i))[1] =
				((const uint64_t*)(inbuf+i))[1] ^ eblk.l[1];
	}

	// Encrypt any remaining partial block
	if (i < size) {
		cblk.w[3] = htob32(i >> 4);
		AES_encrypt(cblk.b, eblk.b, &aeskey);
		for (; i < size; i++)
			((uint8_t*)(outbuf+i))[0] =
				((const uint8_t*)(inbuf+i))[0] ^
				eblk.b[i & 15];
	}
}

int uip_writerawchunk(const void *chunk, size_t size, uipchunkinfo *out_info)
{
	assert(archfd >= 0);
	assert(size <= UIP_MAXCHUNK);

	// Allocate a buffer in which to prepare the chunk write message.
	// (The 4MB maximum chunk size may be a bit large for a stack array.)
	size_t reqsize = sizeof(struct lochdr) + size;
	union locmsg *m = malloc(reqsize);
	if (m == NULL) {
		errno = ENOMEM;
		return -1;
	}
	m->hdr.size = reqsize;
	m->hdr.code = LOCMSG_WRITECHUNK;

	// Copy the chunk data into our request message buffer.
	// A bit unnecessarily inefficient - we could use writev instead...
	memcpy(m->writechunkrequest.data, chunk, size);

	// Ship the chunk off to the storage daemon.
	struct locwritechunkreply rplbuf;
	if (uiplocom_call(m, reqsize, &rplbuf, sizeof(rplbuf)) < 0) {
		free(m);
		return -1;
	}
	assert(rplbuf.hdr.size == sizeof(rplbuf));

	*out_info = rplbuf.info;
	free(m);
	return 0;
}

int uip_writedatachunk(const void *chunk, size_t size,
			uipchunkinfo *out_info, uint8_t *out_key)
{
	assert(archfd >= 0);
	assert(size <= UIP_MAXCHUNK);

	// Allocate a buffer in which to prepare the chunk write message.
	// (The 4MB maximum chunk size may be a bit large for a stack array.)
	size_t reqsize = sizeof(struct lochdr) + size;
	union locmsg *m = malloc(reqsize);
	if (m == NULL) {
		errno = ENOMEM;
		return -1;
	}
	m->hdr.size = reqsize;
	m->hdr.code = LOCMSG_WRITECHUNK;

	// Compute the SHA-384 inner hash of the chunk data
	SHA384_CTX innerctx;
	SHA384_Init(&innerctx);
	SHA384_Update(&innerctx, chunk, size);
	uint8_t innerhash[SHA384_DIGEST_LENGTH];
	SHA384_Final(innerhash, &innerctx);

	// Use the first 256 bits of the inner hash as the AES-256 data key
	memcpy(out_key, innerhash, 32);

	// Encrypt the chunk into the message buffer
	cryptchunk(chunk, m->writechunkrequest.data, size, innerhash);

	// Ship the chunk off
	struct locwritechunkreply rplbuf;
	if (uiplocom_call(m, reqsize, &rplbuf, sizeof(rplbuf)) < 0) {
		free(m);
		return -1;
	}
	assert(rplbuf.hdr.size == sizeof(rplbuf));

	*out_info = rplbuf.info;
	free(m);
	return 0;
}

int uip_writepasschunk(const void *chunk, size_t size, const uint8_t *passkey,
			const uint8_t passchk[UIP_PASSCHKLEN],
			uipchunkinfo *out_info)
{
	assert(archfd >= 0);
	assert(size <= UIP_MAXCHUNK-sizeof(uippasshdr));
	assert((size & 15) == 0);

	// Allocate a buffer in which to prepare the chunk write message.
	// (The 4MB maximum chunk size may be a bit large for a stack array.)
	size_t reqsize = sizeof(struct lochdr) + sizeof(uippasshdr) + size;
	union locmsg *m = malloc(reqsize);
	if (m == NULL) {
		errno = ENOMEM;
		return -1;
	}
	m->hdr.size = reqsize;
	m->hdr.code = LOCMSG_WRITECHUNK;
	uippasshdr *ph = (uippasshdr*)m->writechunkrequest.data;

	// Hash the passkey and the chunk's cleartext to produce the IV.
	// This way the IV is still "unpredictable", as required,
	// to anyone who doesn't already have the passkey and cleartext,
	// but encryption is still convergent given identical parameters.
	SHA384_CTX ivctx;
	SHA384_Init(&ivctx);
	SHA384_Update(&ivctx, passkey, UIP_KEYLEN);
	SHA384_Update(&ivctx, chunk, size);
	uint8_t ivhash[SHA384_DIGEST_LENGTH];
	SHA384_Final(ivhash, &ivctx);
	memcpy(ph->iv, ivhash, 16);

	// CBC-encrypt the pass-check field
	AES_KEY aeskey;
	AES_set_encrypt_key(passkey, 256, &aeskey);
	AES_cbc_encrypt(passchk, ph->chk, 16, &aeskey, ph->iv, 1);

	// CBC-encrypt the chunk data into the buffer after the passhdr
	AES_cbc_encrypt(chunk, (void*)ph+1, size, &aeskey, ph->chk, 1);

	// Ship the chunk off
	struct locwritechunkreply rplbuf;
	if (uiplocom_call(m, reqsize, &rplbuf, sizeof(rplbuf)) < 0) {
		free(m);
		return -1;
	}
	assert(rplbuf.hdr.size == sizeof(rplbuf));

	*out_info = rplbuf.info;
	free(m);
	return 0;
}

int uip_readrawchunk(const uipchunkinfo *info, void *buf)
{
	assert(archfd >= 0);

	if (lseek(archfd, info->archofs, SEEK_SET) < 0)
		return -1;

	// Read and validate the chunk header
	struct uipchunkhdr hdr;
	assert(sizeof(hdr) == 16);
	ssize_t rc = read(archfd, &hdr, 16);
	if (rc < 16) {
		badread:
		if (rc >= 0) {
			inval:
			errno = EINVAL;
		}
		return -1;
	}
	size_t size = btoh32(info->size);
	if (hdr.magic != htob32(UIPCHUNK_MAGIC) ||
			btoh32(hdr.size) != size ||
			hdr.check != *(uint64_t*)info->hash)
		goto inval;

	// Read the chunk cyphertext
	rc = read(archfd, buf, size);
	if (rc < info->size)
		goto badread;

	// Authenticate the chunk's contents via its outer hash
	SHA512_CTX outerctx;
	SHA512_Init(&outerctx);
	SHA512_Update(&outerctx, buf, size);
	uint8_t outerhash[SHA512_DIGEST_LENGTH];
	SHA512_Final(outerhash, &outerctx);
	if (memcmp(outerhash, info->hash, SHA512_DIGEST_LENGTH) != 0) {
		errno = EBADMSG;
		return -1;
	}

	return 0;
}

int uip_readdatachunk(const uipchunkinfo *info, void *buf, const uint8_t *key)
{
	// Read the raw chunk cyphertext
	if (uip_readrawchunk(info, buf) < 0)
		return -1;

	// Decrypt the chunk data in-place
	cryptchunk(buf, buf, info->size, key);

	// Don't verify the inner hash against the key,
	// partly because that would be mostly redundant,
	// and partly because we might want to use random keys
	// rather than convergent inner-hash-based keys sometimes.

	return 0;
}

void uip_passkey(const char *name, const char *password, AES_KEY *out_aeskey)
{
	// Hash the name and password
	SHA384_CTX passctx;
	SHA384_Init(&passctx);
	SHA384_Update(&passctx, name, strlen(name)+1);
	SHA384_Update(&passctx, password, strlen(password)+1);
	uint8_t passhash[SHA384_DIGEST_LENGTH];
	SHA384_Final(passhash, &passctx);

	// Initialize the AES-256 key schedule from the hash
	AES_set_encrypt_key(passhash, 256, out_aeskey);
}

int uip_passcheck(const uipchunkinfo *info, const AES_KEY *aeskey)
{
	uippasshdr *ph = (uippasshdr*)info->head;
	uint8_t outchk[16];
	AES_cbc_encrypt(ph->chk, outchk, 16, aeskey, ph->iv, 0);
	return memcmp(outchk, "UIPjournalPasswd", 16) == 0;
}

int uip_readpasschunk(const uipchunkinfo *info, void *buf,
			const AES_KEY *passkey,
			const uint8_t passchk[UIP_PASSCHKLEN])
{
	// Validate the chunk size
	size_t sz = btoh32(info->size);
	if (sz < sizeof(uippasshdr) || (sz & 15) != 0) {
		errno = EBADMSG;	// can't be a CBC-encrypted chunk!
		return -1;
	}

	// Read the chunk's raw cyphertext into a temporary buffer -
	// can't decrypt in-place since the passhdr needs to be removed.
	uint8_t *cbuf = malloc(sz);
	if (cbuf == NULL) {
		errno = ENOMEM;
		return -1;
	}
	if (uip_readrawchunk(info, cbuf) < 0) {
		free(cbuf);
		return -1;
	}

	// Decrypt and verify the check block
	uint8_t chk[16];
	AES_cbc_encrypt(&cbuf[16], chk, 16, passkey, cbuf, 0);
	if (memcmp(chk, passchk, 16) != 0) {
		free(cbuf);
		errno = EBADMSG;
		return -1;
	}

	// Now decrypt the chunk data into the caller's buffer
	AES_cbc_encrypt(&cbuf[32], buf, sz-32, passkey, &cbuf[16], 0);

	free(cbuf);
	return 0;
}

