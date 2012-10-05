
#include <stdint.h>
#include <algorithm>
#include <arpa/inet.h>	// ntohl

#include "chk64.h"


// Compute a 64-bit checksum over a block of data.
// This checksum algorithm operates on similar principles as Adler-32, except:
//
// - Chk64 operates on sequences of 32-bit data words instead of bytes.
//   (Byte streams must be padded appropriately before using with chk64.)
//
// - Chk64 uses simple 32-bit arithmetic instead of prime-modulus arithmetic.
//   (A prime modulus less than 2^32 would make some input values indistinct,
//   while a prime modulus more than 2^32 would overflow the output fields.)
//
// The data must be 32-bit aligned and its length is specified in 32-bit words.
uint64_t chk64(const uint32_t *data, int nwords, uint64_t init)
{
	uint32_t a = init & 0xffffffff;
	uint32_t b = init >> 32;
	for(; nwords > 0; nwords--) {
		a += ntohl(*data++);
		b += a;
	}
	return ((uint64_t)b << 32) | a;
}


#ifdef CHK64_MAIN

#include <stdio.h>
#include <assert.h>

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

	uint64_t c64 = CHK64_INIT;

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

		c64 = chk64((uint32_t*)buf, act >> 2, c64);
	}

	printf("%016llx  %s\n", c64, name);

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

#endif	// CHK64_MAIN
