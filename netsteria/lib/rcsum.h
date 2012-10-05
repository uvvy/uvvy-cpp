//
// RollingChecksum:
// Simple 32-bite rolling checksum algorithm inspired by Adler-32,
// which can be easily rolled through a buffer at byte granularity.
// Not exactly Adler-32 because we use mod-65536 instead of mod-65521,
// for simplicity and efficiency of the checksum calculations.
// (The nice delayed modulo optimization in Adler-32 doesn't work
// when it's used as a rolling checksum, unfortunately.)
//
#ifndef RCSUM_H
#define RCSUM_H

#include <stddef.h>
#include <stdint.h>

class RollingChecksum
{
	const uint8_t *buf;
	int i, j;
	unsigned a, b;


public:
	// Setup the rolling checksum to use the specified buffer,
	// and initialize it with the specified slab start and end points.
	void init(const void *buf, int slabstart, int slabend);

	// Setup the rolling checksum to use the specified buffer,
	// and initialize it with the first 'slabsize' bytes of data.
	inline void init(const void *buf, int slabsize)
		{ init(buf, 0, slabsize); }

	inline RollingChecksum() { buf = NULL; }
	inline RollingChecksum(const void *buf, int slabstart, int slabend)
		{ init(buf, slabstart, slabend); }
	inline RollingChecksum(const void *buf, int slabsize)
		{ init(buf, 0, slabsize); }

	// Return current slab position or size
	inline int slabStart() { return i; }
	inline int slabEnd() { return j; }
	inline int slabSize() { return j - i; }

	// Advance the slab by one byte and adjust the checksum
	inline void advance() {
			a = a - buf[i] + buf[j];
			b = b - slabSize()*buf[i] + a;
			i++, j++;
		}

	// Advance to a specified start/end position
	inline void advanceStart(int newStart) {
		while (i < newStart) { advance(); } }
	inline void advanceEnd(int newEnd) {
		while (j < newEnd) { advance(); } }

	// Return the computed checksum value for the current slab
	inline uint32_t sum() { return (b << 16) | (a & 0xffff); }


	///// Static methods /////

	// Use a rolling checksum to choose an appropriate chunk size
	// for data starting at 'buf' and containing at least 'maxsize' bytes.
	// The returned chunk size will be between 'minsize' and 'maxsize',
	// and will correspond to the end of the 'minsize'-byte slab
	// having the minimum rolling checksum over all possible positions,
	// ensuring that chosen split points converge on similar content.
	// The 'minsize' should ideally be much more than 256 to ensure that
	// the rolling checksum has a decent distribution - see RFC 3309.
	static int scanChunk(const void *data, int minsize, int maxsize);

	// Compute a one-off checksum over a given data slab -
	// typically only used for sanity-checking.
	static uint32_t sumSlab(const void *data, int slabsize);
};

#endif	// RCSUM_H
