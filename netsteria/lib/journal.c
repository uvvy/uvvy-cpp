
#include <stdlib.h>

#include <openssl/aes.h>

#include <uip/journal.h>

#include "sha2.h"


static void freeent(uipentry *ent)
{
	if (ent->chunkbuf) {
		free(ent->chunkbuf);

		ent->crefs = NULL;	// points into chunk buffer

		for (int i = 0; i < ent->nnodes; i++) {
			uipnode *n = ent->nodes[i];
			if (n == NULL)
				continue;
			n->data = NULL;	// points into chunk buffer
		}
	}

	if (ent->crefs)
		free(ent->crefs);
	if (ent->nodes)
		free(ent->nodes);
	free(ent);
}

static uipentry *findentry(uipjournal *jrn, const uipchunkinfo *info)
{
	return NULL;	// XXX
}

static int decodenode(uipentry *ent, int nodenum, void *ndata, size_t nsize)
{
	assert(nodenum < ent->nnodes);
	assert(ent->nodes[nodenum] == NULL);

	// XX check already exists

	uipnode *n = calloc(1, sizeof(*n));
	if (n == NULL) {
		errno = ENOMEM;
		return -1;
	}
	ent->nodes[nodenum] = n;

	// Get the node type
	if (nsize < 2)
		goto corrupt;
	uint16_t ntype = btoh16(*(uint16*)ndata);
	if (ntype == NTYPE_INVALID)
		goto corrupt;
	n->type = ntype;

	// Leave everything else to be parsed later
	n->data = ndata;
	n->size = nsize;

	return 0;

	corrupt:
	assert(0);	// XXX
}

static int decodenodes(uipentry *ent, void *nodesblk, size_t blksize)
{
	if (jrn->nodes != NULL)
		goto corrupt;	// duplicate nodes block

	// Grab the node count
	if (blksize < 4+4)
		goto corrupt;
	uint32_t nnodes = btoh32(*(uint32_t*)(p+4));
	if ((nnodes < 1) || (nnodes > ((size-4-4)/4)))
		goto corrupt;

	// The node offset table contains the offsets in the block
	// of all but the first node;
	// the first node always starts right affter the offset table itself.
	// The offsets are always sorted in ascending order.
	ent->nodes = calloc(nnodes, 4);
	if (ent->nodes == NULL) {
		errno = ENOMEM;
		return -1;
	}
	ent->nnodes = nnodes;

	// Collect the nodes
	uint32_t ofs = 4+4+(nnodes-1)*4;
	for (int i = 0; i < nnodes-1; i++) {
		nextofs = btoh32(*(uint32_t*)(p+4+4+i*4));
		if (nextofs <= ofs)
			goto corrupt;
		if (nextofs >= blksize)
			goto corrupt;
		if (decodenode(ent, i, nodesblk+ofs, nextofs-ofs) < 0)
			return -1;
		ofs = nextofs;
	}
	if (decodenode(ent, nnodes-1, ofs, blksize-ofs) < 0)
		return -1;

	return 0;

	corrupt:
	assert(0);	// XX
}

// Always "consumes" the supplied chunk buffer,
// free'ing it when no longer used.
static uipentry *decodeentry(uipjournal *jrn, void *chunk, size_t size)
{
	uipentry *ent = calloc(1, sizeof(*ent));
	if (ent == NULL) {
		free(chunk);
		errno = ENOMEM;
		return NULL;
	}
	ent->chunkbuf = chunk;

	// Scan for the crucial metadata blocks in the chunk.
	// The actual entry data starts after the uippasshdr.
	void *p = chunk+sizeof(uippasshdr);
	void *pe = chunk+size;
	while (p < pe) {
		uint32_t blkhdr = btoh32(*(uint32_t*)p);
		int type = blkhdr >> 24;
		size_t size = blkhdr & 0xffffff;
		if (size < 4 || p+size > pe)
			goto corrupt;

		switch (type) {
		case UIPENTBLK_INVALID:
			goto corrupt;

		case UIPENTBLK_STAMP:
			if ((intptr_t)(p+4) & 7)
				goto corrupt;
			if (size < 4+8)
				goto corrupt;
			ent->time = btoh64(*(uint64_t*)(p+4));
			// XX check duplicate
			// XX machine
			break;

		case UIPENTBLK_CREFS:
			if (((size-4) % sizeof(uipchunkref)) != 0)
				goto corrupt;
			if (jrn->crefs != NULL)
				goto corrupt;	// duplicate crefs block
			jrn->crefs = (uipchunkref*)(p + 4);
			jrn->ncrefs = (size-4) / sizeof(uipchunkref);
			break;

		case UIPENTBLK_NODES:
			if (decodenodes(ent, p, size) < 0)
				goto fail;
			break;

		case UIPENTBLK_PAD:
		default:
			// just ignore block
		}

		// Move to next metadata block
		p += (size + 3) & ~3;
	}

	corrupt:
	assert(0);	// XXX

	fail:
	freeent(ent);
	return NULL;
}

static uipentry *loadentry(uipjournal *jrn, const uipchunkinfo *info,
				uint8_t *datakey)
{
	// XX make sure entry isn't already loaded?
	uipentry *ent = findentry(jrn, info);
	if (ent != NULL)
		return ent;

	void *buf = malloc(info->size);
	if (buf == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	// Read in and decrypt the chunk
	if (uip_readchunk(info, buf, datakey) < 0) {
		free(buf);
		return NULL;
	}

	return decodeentry(jrn, buf, size);
}

static uipentry *loadentrypass(uipjournal *jrn, const uipchunkinfo *info,
				const AES_KEY *aeskey)
{
	// XX make sure entry isn't already loaded?
	uipentry *ent = findentry(jrn, info);
	if (ent != NULL)
		return ent;

	void *buf = malloc(info->size);
	if (buf == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	// Read in and decrypt the chunk
	if (uip_readchunkpass(info, buf, aeskey) < 0) {
		free(buf);
		return NULL;
	}

	return decodeentry(jrn, buf, size);
}

uipjournal *uipjournal_open(
	const char *journalname,
	const char *password,
	int create,
	int (*progress)(int fraction, void *data),
	void *progressdata)
{
	// XX For now we just start at the beginning of the archive
	// and read the full log to construct the internal state;
	// soon we'll need to cache recent history though
	// and incrementally update it when we re-open an archive.

	// Form the AES key for password-decrypting journal entries
	AES_KEY aeskey;
	uip_passkey(journalname, password, &aeskey);

	// Start building the journal structure
	uipjournal *jrn = calloc(1, sizeof(*j));
	if (jrn == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	// Scan the archive for entries comprising this journal
	if (uipjournal_update(jrn, progress, progressdata) < 0) {
		uipjournal_close(jrn);
		return NULL;
	}

	// XX make sure something was found

	return jrn;
}

void uipjournal_update(uipjournal *journal,
	int (*progress)(int fraction, void *data),
	void *progressdata)
{
	// Scan through the archive for decryptable journal entries
	uipchunkid id = 0;
	const int nchunks = 10;
	uipchunkinfo info[nchunks];
	while (1) {
		int rc = uip_chunkinfo(id, nchunks, info);
		if (rc < 0) {
			uipjournal_close(jrn);
			return NULL;
		}
		if (rc == 0)
			break;
		assert(rc <= nchunks);
		for (int i = 0; i < rc; i++) {

			// Use the 32-byte 'head' field in the chunk info
			// to determine if it's keyed to this name/password.
			if (!uip_passcheck(&info[i], &aeskey))
				continue;

			if (loadentrypass(jrn, &info[i], &aeskey) < 0) {
				uipjournal_close(jrn);
				return NULL;
			}
		}
	}

}

void uipjournal_close(uipjournal *jrn)
{
	// XX free entries, nodes, etc.

	free(jrn);
}

