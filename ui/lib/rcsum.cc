
#include <assert.h>

#include "rcsum.h"


// Initialize the rolling checksum over the first data slab.
void RollingChecksum::init(const void *buffer, int slabstart, int slabend)
{
	assert(slabstart < slabend);

	buf = (const uint8_t*)buffer;
	a = 0;
	b = 0;
	i = slabstart;
	for (j = i; j < slabend; j++) {
		a += buf[j];
		b += a;
	}
}

int RollingChecksum::scanChunk(const void *data, int minsize, int maxsize)
{
	assert(maxsize >= minsize);

	// Initialize the checksum over the first minsize bytes.
	RollingChecksum rc(data, minsize);

	// Now compute the rolling checksum for successive minsize-blocks,
	// recording the position resulting in the lowest checksum.
	uint32_t selsum = rc.sum();
	int selsize = minsize;
	while (rc.slabEnd() < maxsize) {

		// Advance and adjust the checksum
		rc.advance();
		uint32_t newsum = rc.sum();

		// Pick the end of the minsize-block with lowest checksum
		// to be the end of the selected chunk.
		// Break checksum ties in favor of larger chunks.
		if (newsum <= selsum) {
			selsum = newsum;
			selsize = rc.slabEnd();
		}

		// This assert is very bad for performance;
		// only uncomment it for sanity-checking purposes.
		//assert(rc.sum() == sumSlab(rc.buf+rc.slabStart(), minsize));
	}

	return selsize;
}

uint32_t RollingChecksum::sumSlab(const void *data, int slabsize)
{
	RollingChecksum rc(data, slabsize);
	return rc.sum();
}

