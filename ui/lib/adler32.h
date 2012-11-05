#ifndef ADLER32_H
#define ADLER32_H

#include <stdint.h>


// Standard Adler32 checksum initialization value: b = 0, a = 1
#define ADLER32_INIT	0x00000001


// Compute an Adler-32 checksum over a given block of data.
uint32_t adler32(const void *data, int len, uint32_t init = ADLER32_INIT);

// Compose the Adler-32 checksums for two data blocks
// into the checksum for the concatenation of the two blocks.
// To do this we only need the length of the second block.
uint32_t adler32cat(uint32_t sum1, uint32_t sum2, uint64_t len2);


#endif	// ADLER32_H
