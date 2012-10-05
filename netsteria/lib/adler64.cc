
#include <stdint.h>
#include <algorithm>
#include <arpa/inet.h>	// ntohl

#include "adler64.h"


#define MODA	6442450941ull	// MODB * 3
#define	MODB	2147483647ull	// Greatest prime less than 2^31


// Compute an Adler-64 checksum (my own invention) over a block of data.
// This checksum algorithm operates on similar principles as Adler-32,
// except that it operates on 32-bit words at a time instead of bytes,
// computes sums uses 64-bit accumulators, and produces a 64-bit output.
// To ensure that all 2^32 values of each input word are distinguishable,
// the low counter ('a') uses a 33-bit modulus
// leaving 31 bits for the high counter ('b').
// The high counter uses a prime modulus (MODB) of 2^31-1,
// and the low counter uses a modulus (MODA) of MODB * 3
// to preserve the relative modularity of the two counters.
// The data must be 32-bit aligned its length must be a multiple of 32 bits.
uint64_t adler64(const void *data, int len, uint64_t init)
{
	const uint32_t *d = (const uint32_t*)data;
	len >>= 2;
	uint64_t a = init & 0x1ffffffffull;
	uint64_t b = init >> 33;
	for (int j = 0; j < len; ) {

		// We can sum up to 92679 consecutive words at a time
		// before our unsigned 64-bit accumulators overflow.
		for (int k = std::min(j+92670, len); j < k; j++) {
			a += ntohl(*d++);
			b += a;
		}
		a %= MODA;
		b %= MODB;
	}
	return (b << 33) | a;
}


#ifdef TEST

#include <stdio.h>
#include <assert.h>

#include "adler32.h"

#define BUFSIZE 65536

char buf[BUFSIZE];

void sumfile(const char *name)
{
	FILE *in;
	if (strcmp(name, "-") == 0)
		in = stdin;
	else
		in = fopen(name, "rb");
	if (in == NULL) {
		perror("Can't open input file");
		return;
	}

	uint32_t a32 = ADLER32_INIT;
	uint64_t a64 = ADLER64_INIT;

	while (!feof(in)) {
		size_t act = fread(buf, 1, BUFSIZE, in);
		if (act < BUFSIZE) {
			if (ferror(in)) {
				perror("Error reading input file");
				fclose(in);
				return;
			}
			assert(feof(in));

			// Add byte padding
			buf[act++] = 0x80;
			while (act & 3)
				buf[act++] = 0;
		}

		a32 = adler32(buf, act, a32);
		a64 = adler64(buf, act, a64);
	}

	printf("%08x  %016llx  %s\n", a32, a64, name);

	if (in != stdin)
		fclose(in);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file> [...]\n"
				"  ('-' for standard input)\n", argv[0]);
		exit(1);
	}

	for (int i = 1; i < argc; i++)
		sumfile(argv[i]);
}

#endif	// TEST
