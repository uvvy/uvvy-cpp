#ifndef chk64_H
#define chk64_H

#include <stdint.h>


// Standard chk64 checksum initialization value: b = 0, a = 1
#define CHK64_INIT	((uint64_t)0x0000000000000001)


// Compute a chk64 checksum over a given block of data.
uint64_t chk64(const uint32_t *words, int nwords, uint64_t init = CHK64_INIT);


#endif	// ADLER64_H

