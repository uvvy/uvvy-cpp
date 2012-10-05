
#include <inttypes.h>
#include <errno.h>

#include <uip/base32.h>

// Decoding table for our base32 encoding scheme
static const signed char dectab[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1,		// Control chars
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,		// Space-"'"
	-1, -1, -1, -1, -1, -1, -1, -1,		// '('-'/'
	 0,  1,  2,  3,  4,  5,  6,  7,		// 0-7
	 8,  9, -1, -1, -1, -1, -1, -1,		// 8-'?'
	-1, 10, 11, 12, 13, 14, 15, 16,		// '@', A-G
	17,  1, 18, 19,  1, 20, 21,  0,		// H-O: I,L = 1, O = 0
	22, 23, 24, 25, 26, 27, 28, 29,		// P-W
	30, 31,  2, -1, -1, -1, -1, -1,		// X-'_': Z = 2
	-1, 10, 11, 12, 13, 14, 15, 16,		// '@', a-g
	17,  1, 18, 19,  1, 20, 21,  0,		// h-o: i,l = 1, o = 0
	22, 23, 24, 25, 26, 27, 28, 29,		// p-w
	30, 31,  2, -1, -1, -1, -1, -1,		// x-DEL: z = 2

	-1, -1, -1, -1, -1, -1, -1, -1,		// Latin-1 ...
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
};


void uip_base32encode(const void *buf, size_t size, char *out_str, int dashes)
{
	static const char conv[32] = "0123456789ABCDEFGHJKMNPQRSTUVWXY";

	// Handle complete 40-bit groups
	const uint8_t *b = (const uint8_t*)buf;
	char *s = out_str;
	int i = 0;
	while (i+5 <= size) {
		uint32_t bhi = ((uint32_t)b[i] << (32-24)) |
				((uint32_t)b[i+1] << (24-24));
		uint32_t blo = (bhi << 24) |
				((uint32_t)b[i+2] << 16) |
				((uint32_t)b[i+3] << 8) |
				((uint32_t)b[i+4] << 0);
		s[0] = conv[(bhi >> (35-24)) & 0x1f];
		s[1] = conv[(bhi >> (30-24)) & 0x1f];
		s[2] = conv[(blo >> 25) & 0x1f];
		s[3] = conv[(blo >> 20) & 0x1f];
		s[4] = conv[(blo >> 15) & 0x1f];
		s[5] = conv[(blo >> 10) & 0x1f];
		s[6] = conv[(blo >> 5) & 0x1f];
		s[7] = conv[(blo >> 0) & 0x1f];
		i += 5;
		s += 8;
		if (dashes)
			*s++ = '-';
	}

	if (i == size) {

		// Data was an even number of 40-bit groups in size.
		if (dashes)
			s--;	// eliminate last '-'
		*s = 0;

	} else {

		// Handle the final partial group
		uint32_t bhi = (uint32_t)b[i++] << (32-24);
		s[2] = 0;
		if (i < size) {
			bhi |= (uint32_t)b[i++] << (24-24);
			uint32_t blo = bhi << 24;
			s[4] = 0;
			if (i < size) {
				blo |= (uint32_t)b[i++] << 16;
				s[5] = 0;
				if (i < size) {
					blo |= (uint32_t)b[i++] << 8;
					s[7] = 0;
					if (i < size) {
						blo |= (uint32_t)b[i++] << 0;
						s[8] = 0;
						s[7] = conv[(blo >> 0) & 0x1f];
					}
					s[5] = conv[(blo >> 10) & 0x1f];
					s[6] = conv[(blo >> 5) & 0x1f];
				}
				s[4] = conv[(blo >> 15) & 0x1f];
			}
			s[2] = conv[(blo >> 25) & 0x1f];
			s[3] = conv[(blo >> 20) & 0x1f];
		}
		s[0] = conv[(bhi >> (35-24)) & 0x1f];
		s[1] = conv[(bhi >> (30-24)) & 0x1f];
	}
}

int uip_base32decode(const char *str, void *out_buf, size_t *inout_size)
{
	const char *s = str;
	uint8_t *b = (uint8_t*)out_buf;
	uint8_t *bmax = b + *inout_size;

#define DECODE(c)	\
	c = *s++;	\
	while (c == '-') c = *s++;	\
	c = dectab[(uint8_t)c];	\
	if (c < 0) break;

#define OUTPUT(v) \
	if (b == bmax) { errno = EINVAL; return -1; }	\
	*b++ = (v);

	while (1) {
		signed char c;

		// Decode first binary output byte in a 40-bit group
		DECODE(c);
		uint32_t bhi = (uint32_t)c << (35-24);
		DECODE(c);
		bhi |= (uint32_t)c << (30-24);
		OUTPUT(bhi >> (32-24));

		// Second output byte
		DECODE(c);
		uint32_t blo = (bhi << 24) | (uint32_t)c << 25;
		DECODE(c);
		blo |= (uint32_t)c << 20;
		OUTPUT(blo >> 24);

		// Third output byte
		DECODE(c);
		blo |= (uint32_t)c << 15;
		OUTPUT(blo >> 16);

		// Fourth output byte
		DECODE(c);
		blo |= (uint32_t)c << 10;
		DECODE(c);
		blo |= (uint32_t)c << 5;
		OUTPUT(blo >> 8);

		// Fifth output byte
		DECODE(c);
		blo |= (uint32_t)c << 0;
		OUTPUT(blo >> 0);
	}

	// Encountered terminator or illegal character.
	// Return actual amount of input string and buffer space consumed.
	*inout_size = b - (uint8_t*)out_buf;
	return s - str;
}

