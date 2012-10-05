
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#include <openssl/aes.h>

#include <uip/archive.h>

#include "hub.h"
#include "sha2.h"
#include "byteorder.h"


static int archfd = -1;

// Memory-mapped file containing the archive chunk index
static int chunkidxfd = -1;
//static uipchunkinfo *chunkidx;
//static uint64_t nchunks;

// Memory-mapped file containing the archive hash index
static int hashidxfd = -1;
static uipchunkinfo *hashidx;
static size_t hashidxlen;		// always a power of two

#define HASHIDX_MINLEN			1024

uipchunkid nextchunkid;


uint8_t uip_archmagic[20] = {
	'U', 'I', 'P', 'a', 'r', 'c', 'h', 'i', 'v', 'e',
	26,	// Windows end-of-file marker (CTRL-Z)
	0x00,	// Make sure tools recognize the file as binary
	0x1d, 0x69, 0x78, 0x5b, 0x9e, 0xf1, 0x43, 0x8c,	// Random bytes
};

static uint8_t zeros[16];


void archive_init(const char *uipdir)
{
	assert(archfd < 0);

	// Open or create the archive
	char name[strlen(uipdir)+32];
	sprintf(name, "%s/archive", uipdir);
	archfd = open(name, O_RDWR | O_CREAT | O_APPEND, 0666);
	if (archfd < 0)
		fatal("can't open archive file '%s': %s",
			name, strerror(errno));

	// Make sure we have exclusive access to the archive
	struct flock flk;
	flk.l_type = F_WRLCK;
	flk.l_whence = SEEK_SET;
	flk.l_start = 0;
	flk.l_len = 0;
	if (fcntl(archfd, F_SETLK, &flk) < 0) {
		if (errno == EACCES || errno == EAGAIN)
			fatal("can't obtain exclusive access to archive '%s': "
				"another UIP hub may already be running.",
				name);
		fatal("can't obtain exclusive access to archive '%s': %s",
			name, strerror(errno));
	}

	// Read the archive header
	struct uiparchhdr hdr;
	ssize_t rc = read(archfd, &hdr, sizeof(hdr));
	if (rc < 0)
		fatal("error reading archive header from '%s': %s",
			name, strerror(errno));
	if (rc == 0) {
		// We're creating a new archive - write a fresh archive header.
		memcpy(hdr.magic, uip_archmagic, 20);

		hdr.optsize = htob32(0);	// no options for now
		uint8_t hash[SHA512_DIGEST_LENGTH];
		SHA512_CTX ctx;
		SHA512_Init(&ctx);
		SHA512_Final(hash, &ctx);
		hdr.optcheck = *(uint64_t*)hash;

		rc = write(archfd, &hdr, sizeof(hdr));
		if (rc < 0)
			fatal("error writing archive header to '%s': %s",
				name, strerror(errno));
	} else {
		// Validate the existing archive header
		if (memcmp(hdr.magic, uip_archmagic, 20) != 0)
			fatal("archive '%s' is corrupt: invalid header magic",
				name);
		if (hdr.optsize != 0)
			fatal("archive '%s' has unexpected header options",
				name);	// XX
	}

	// Open or create the archive chunk index
	sprintf(name, "%s/archindex", uipdir);
	chunkidxfd = open(name, O_RDWR | O_CREAT | O_APPEND, 0666);
	if (chunkidxfd < 0)
		fatal("can't open archive chunk index file '%s': %s",
			name, strerror(errno));

	// Determine the number of chunks already in the archive
	off_t idxsize = lseek(chunkidxfd, 0, SEEK_END);
	if (idxsize < 0)
		fatal("can't determine size of chunk index '%s': %s",
			name, strerror(errno));
	assert(sizeof(uipchunkinfo) == 128);
	if ((idxsize & 127) != 0)
		fatal("chunk index file '%s' has invalid length "
			"and is probably corrupt", name);
	nextchunkid = idxsize / 128;

	// Open or create the archive hash table
	sprintf(name, "%s/hashindex", uipdir);
	hashidxfd = open(name, O_RDWR | O_CREAT, 0666);
	if (hashidxfd < 0)
		fatal("can't open archive hash index file '%s': %s",
			name, strerror(errno));

	// Check the current hash table size;
	// grow it to its minimum size if it's new.
	off_t htsize = lseek(hashidxfd, 0, SEEK_END);
	if (htsize < 0)
		fatal("can't determine length of hash index '%s': %s",
			name, strerror(errno));
	assert(sizeof(uipchunkinfo) == 128);
	if (htsize == 0) {
		if (ftruncate(hashidxfd, HASHIDX_MINLEN * 128) < 0)
			fatal("error initializing hash index '%s': %s",
				name, strerror(errno));
		htsize = HASHIDX_MINLEN * 128;
	}
	hashidxlen = htsize / 128;
	if (((htsize & 127) != 0) ||
			(hashidxlen < HASHIDX_MINLEN) ||
			((hashidxlen & (hashidxlen-1)) != 0))
		fatal("hash index file '%s' has invalid length "
			"and is probably corrupt",	// XX say how
			name);

	// Map the hash index into memory
	void *ptr = mmap(NULL, htsize, PROT_READ | PROT_WRITE, MAP_SHARED,
			hashidxfd, 0);
	if (ptr == MAP_FAILED)
		fatal("error memory-mapping hash index '%s': %s",
			name, strerror(errno));
	hashidx = (uipchunkinfo*)ptr;
}

int archive_lookupid(uipchunkid id, uipchunkinfo *out_info)
{
	off_t idxpos = id * sizeof(uipchunkinfo);
	if ((idxpos / sizeof(uipchunkinfo)) != id) {
		errno = EINVAL;		// integer overflow
		return -1;
	}

	if (lseek(chunkidxfd, idxpos, SEEK_SET) < 0)
		return -1;

	ssize_t rc = read(chunkidxfd, out_info, sizeof(uipchunkinfo));
	if (rc < 0)
		return -1;
	if (rc < (ssize_t)sizeof(uipchunkinfo)) {
		errno = EINVAL;		// chunk ID too high
		return -1;
	}

	return 0;
}

int archive_lookuphash(const uint8_t *outerhash, uipchunkinfo *out_info)
{
	assert(hashidx != NULL);
	assert(hashidxlen >= HASHIDX_MINLEN);
	assert((hashidxlen & (hashidxlen-1)) == 0);	// must be power of two

	size_t hashkey = btoh64(*(uint64_t*)outerhash) & (hashidxlen-1);
	while (hashidx[hashkey].size != 0) {
		if (memcmp(hashidx[hashkey].hash, outerhash, 64) == 0) {
			// found it!
			if (out_info != NULL)
				*out_info = hashidx[hashkey];
			return 0;
		}
		hashkey = (hashkey + 1) & (hashidxlen-1);
	}

	errno = ENOENT;
	return -1;
}

int archive_readchunkpart(const uipchunkinfo *info, void *buf,
				size_t chunkofs, size_t bufsize)
{
	size_t chunksize = btoh32(info->size);
	if (chunkofs + bufsize > chunksize) {
		errno = EINVAL;
		return -1;
	}

	off_t archofs = btoh64(info->archofs) + sizeof(uipchunkhdr) + chunkofs;
	if (lseek(archfd, archofs, SEEK_SET) < 0)
		return -1;

	ssize_t rc = read(archfd, buf, bufsize);
	if (rc < 0)
		return -1;
	if (rc < (ssize_t)bufsize) {
		errno = EINVAL;	// invalid info block apparently
		return -1;
	}

	return 0;
}

int archive_readchunk(const uipchunkinfo *info, void *buf)
{
	size_t size = btoh32(info->size);
	return archive_readchunkpart(info, buf, 0, size);
}

static int growhashidx()
{
	assert(0);	// XXX
	return -1;
}

int archive_writechunk(const void *buf, size_t size, const uint8_t *hash,
			uipchunkinfo *out_info)
{
	assert(archfd >= 0);
	assert(size > 0);

	if (size > UIP_MAXCHUNK) {
		errno = EINVAL;
		return -1;
	}

	// Start setting up the chunk info block
	uipchunkinfo info;
	memset(&info, 0, sizeof(info));
	if (size < 32) {
		memcpy(info.head, buf, size);
		memset(info.head+size, 0, 32-size);
	} else
		memcpy(info.head, buf, 32);
	info.size = htob32(size);

	// Compute the SHA-512 outer hash
	SHA512_CTX outerctx;
	SHA512_Init(&outerctx);
	SHA512_Update(&outerctx, (uint8_t*)buf, size);
	SHA512_Final(info.hash, &outerctx);

	// Verify it against the expected hash, if provided
	if (hash != NULL) {
		if (memcmp(hash, info.hash, SHA512_DIGEST_LENGTH) != 0) {
			errno = EBADMSG;
			return -1;
		}
	}

	// See if this chunk is already in the archive:
	// if so, just return the chunk's existing info struct
	if (archive_lookuphash(info.hash, out_info) == 0)
		return 0;

	// Find the offset the new chunk will have in the archive
	off_t archofs = lseek(archfd, 0, SEEK_END);
	if (archofs < 0)
		return -1;
	assert((archofs & 15) == 0);
	info.archofs = htob64(archofs);

	// Find the chunk ID the new chunk will have
	off_t idxofs = lseek(chunkidxfd, 0, SEEK_END);
	if (idxofs < 0)
		return -1;
	assert(sizeof(info) == 128);
	assert((idxofs & 127) == 0);
	uipchunkid chunkid = idxofs / 128;
	info.chunkid = htob64(chunkid);

	// Grow the hash index if it's getting too small
	if (chunkid > hashidxlen * 2 / 3)
		if (growhashidx() < 0)
			return -1;

	// Write the chunk to the archive
	struct uipchunkhdr hdr;
	hdr.magic = htob32(UIPCHUNK_MAGIC);	// "UIPc"
	hdr.size = htob32(size);
	hdr.check = *(uint64_t*)info.hash;
	assert(sizeof(hdr) == 16);
	ssize_t rc;
	if ((rc = write(archfd, &hdr, 16)) < 16) {
		wrerr:
		// Remove any partial chunk we might have written,
		// to avoid corrupting the archive if we run out of space.
		ftruncate(archfd, archofs);
		if (rc >= 0)
			errno = ENOSPC;
		return -1;
	}
	if ((rc = write(archfd, buf, size)) < (ssize_t)size)
		goto wrerr;
	if (size & 15) {
		// Pad the chunk to a 16-byte boundary
		ssize_t pad = 16 - (size & 15);
		if ((rc = write(archfd, zeros, pad)) < pad)
			goto wrerr;
	}

	// Write the info block to the chunk index
	if ((rc = write(chunkidxfd, &info, sizeof(info)))
			< (ssize_t)sizeof(info)) {
		ftruncate(chunkidxfd, idxofs);
		goto wrerr;
	}

	// Insert the info block in the hash index
	size_t hashkey = btoh64(*(uint64_t*)info.hash) & (hashidxlen-1);
	size_t origkey = hashkey;
	while (hashidx[hashkey].size != 0) {
		// hash collision
		hashkey = (hashkey + 1) & (hashidxlen-1);
		assert(hashkey != origkey);
	}
	hashidx[hashkey] = info;

	nextchunkid = chunkid+1;

	if (out_info != NULL)
		*out_info = info;
	return 0;
}

