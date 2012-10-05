/*
 * Structured Stream Transport
 * Copyright (C) 2006-2008 Massachusetts Institute of Technology
 * Author: Bryan Ford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include <stdint.h>
#include <algorithm>
#include <arpa/inet.h>	// ntohl

#include "chk32.h"

using namespace SST;


// Compute a 32-bit checksum over a block of data.
// This checksum algorithm operates on similar principles as Adler-32, except:
//
// - Chk32 operates on sequences of 16-bit data words instead of bytes,
//   so that the checksum has better entropy when used on short strings.
//   Byte strings are padded with a trailing 0x80 byte or a 0x8000 word.
//
// - Chk32 uses a prime modulus of 65537 (0x10001) for its arithmetic,
//   so that every input data value from 0 through 65535 is distinguishable.
//   The final checksum uses only the low 16 bits of the two counters.
//
void Chk32::update16(const uint16_t *buf, int nwords)
{
	// With our 64-bit counters we can process 23726746 16-bit words
	// before risk of overflow.
	while (nwords > run) {
		for (int i = run; i > 0; i--) {
			a += (uint16_t)ntohs(*buf++);
			b += a;
		}
		nwords -= run;
		run = MaxRun;
		a %= Modulus;
		b %= Modulus;
	}

	run -= nwords;
	for(; nwords > 0; nwords--) {
		a += (uint16_t)ntohs(*buf++);
		b += a;
	}
}

uint32_t Chk32::final16()
{
	a %= Modulus;
	b %= Modulus;
	return (a & 0xffff) | (b << 16);
}

void Chk32::update(const void *buf, size_t size)
{
	if (size <= 0)
		return;

	const uint8_t *bytes = (const uint8_t*)buf;

	// Process any current odd character
	if (haveodd) {
		oddbuf.b[1] = *bytes++;
		size--;
		update16(&oddbuf.w, 1);
		haveodd = false;
	}

	// Process whole 16-bit words
	update16((const uint16_t*)bytes, size >> 1);

	// Save any remaining odd character
	if (size & 1) {
		haveodd = true;
		oddbuf.b[0] = bytes[size-1];
	}
}

uint32_t Chk32::final()
{
	// Pad the byte stream appropriately
	if (haveodd) {
		oddbuf.b[1] = 0x80;
	} else {
		oddbuf.b[0] = 0x80;
		oddbuf.b[1] = 0x00;
	}
	update16(&oddbuf.w, 1);

	// Return the final checksum
	return final16();
}


#ifdef CHK32_MAIN

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

	Chk32 c;
	while (!feof(in)) {
		size_t act = fread(buf, 1, BUFSIZE, in);
		if (act < BUFSIZE) {
			if (ferror(in)) {
				perror("Error reading input file");
				fclose(in);
				return;
			}
			assert(feof(in));
		}
		c.update(buf, act);
	}

	printf("%08x  %s\n", c.final(), name);

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

#endif	// CHK32_MAIN
