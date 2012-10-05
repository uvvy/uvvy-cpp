/*
 * Simple, generic hash table manager used throughout uipd.
 */
#ifndef UIPD_HASH_H
#define UIPD_HASH_H

#include <inttypes.h>

struct HashEnt
{
	const void *key;
	size_t keylen;
	size_t hash;

	inline HashEnt() { key = 0; }
};

struct HashTab
{
	struct HashEnt **ht;
	size_t htlen;
	size_t htmask;	/* == htlen-1 */
	size_t nents;


	HashTab();
	~HashTab();

	/* Our hash function.
	   Could be made virtual if we decide it needs to vary. */
	static size_t hashfunc(const void *key, size_t keylen);

	/* Lookup the entry for a specified key,
	 * returning NULL if no such entry exists. */
	struct HashEnt *lookup(const void *key, size_t keylen);

	/* Insert an entry into the hash table with the specified key.
	 * The memory containing the key MUST remain allocated and unmodified
	 * for as long as the entry is in the hash table.
	 * Throws ENOMEM if not enough memory to grow the hash table,
	 * EADDRINUSE if an entry with the same key already exists. */
	void insert(struct HashEnt *e, const void *key, size_t keylen);

	/* Remove an entry into a hash table.
	   Throws EINVAL if the entry is not found. */
	void remove(struct HashEnt *e);

	/* Remove all the entries in the hash table.
	   If 'del' is nonzero, then delete them as well. */
	void clear(int del);

	/* Returns true if inserting an entry with the specified key
	   would cause a collision in the hash table.
	   Can be used to prevent unnecessary hash collisions,
	   if keys are chosen randomly for example. */
	int collides(const void *key, size_t keylen);

	/* Resize the hash table to a given number of entries.
	 * The new length must be a power of two
	 * and must be large enough to hold all existing entries in use. */
	void resize(size_t newlen);
};

#endif /* UIPD_HASH_H */
