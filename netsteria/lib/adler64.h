#ifndef ADLER64_H
#define ADLER64_H

#include <stdint.h>


// Standard Adler64 checksum initialization value: b = 0, a = 1
#define ADLER64_INIT	((uint64_t)0x0000000000000001)


// Compute an Adler-64 checksum over a given block of data.
uint64_t adler64(const void *data, int len, uint64_t init = ADLER64_INIT);


#endif	// ADLER64_H
