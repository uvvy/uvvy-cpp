
#include <stdint.h>
#include <algorithm>

#include "adler32.h"


// Compute an Adler-32 checksum over a given block of data.
uint32_t adler32(const void *data, int len, uint32_t init)
{
	const uint8_t *d = (const uint8_t*)data;
	unsigned a = init & 0xffff;
	unsigned b = init >> 16;
	for (int j = 0; j < len; ) {

		// We can sum up to 5552 bytes at a time (RFC 1950)
		// before our unsigned 32-bit accumulators overflow.
		for (int k = std::min(j+5550, len); j < k; j++) {
			a += *d++;
			b += a;
		}
		a %= 65521;
		b %= 65521;
	}
	return (b << 16) | a;
}


// Compose the Adler-32 checksums for two data blocks
// into the checksum for the concatenation of the two blocks.
// To do this we only need the length of the second block.
uint32_t adler32cat(uint32_t sum1, uint32_t sum2, uint64_t len2)
{
	// Modulo the length before multiplying, to avoid overflow.
	unsigned len2mod = len2 % 65521;

	unsigned a1 = sum1 & 0xffff; unsigned a2 = sum2 & 0xffff;
	unsigned b1 = sum1 >> 16; unsigned b2 = sum2 >> 16;

	unsigned a = (a1 + a2 - 1) % 65521;
	unsigned b = ((a1 * len2mod) + b1 + b2 - len2mod) % 65521;

	return (b << 16) | a;
}


