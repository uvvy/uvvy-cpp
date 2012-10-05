//
// $Id$
//
// Copyright (c) 2006 Bryan Ford (baford@mit.edu)
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <sst/base64.h>


// Decoding table for UIA's base64 encoding scheme.
// Accepts either '+' or '-' for character 62,
// and either '/' or '_' for character 63 -
// i.e., either "standard" or "URL-friendly" base64 encodings.
static const signed char dectab[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1,		// Control chars
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,		// Space-"'"
	-1, -1, -1, 62, -1, 62, -1, 63,		// '('-'/'
	52, 53, 54, 55, 56, 57, 58, 59,		// 0-7
	60, 61, -1, -1, -1, -1, -1, -1,		// 8-'?'
	-1,  0,  1,  2,  3,  4,  5,  6,		// '@', A-G
	 7,  8,  9, 10, 11, 12, 13, 14,		// H-O
	15, 16, 17, 18, 19, 20, 21, 22,		// P-W
	23, 24, 25, -1, -1, -1, -1, 63,		// X-'_'
	-1, 26, 27, 28, 29, 30, 31, 32,		// '@', a-g
	33, 34, 35, 36, 37, 38, 39, 40,		// h-o
	41, 42, 43, 44, 45, 46, 47, 48,		// p-w
	49, 50, 51, -1, -1, -1, -1, -1,		// x-DEL

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


int sst_base64encode(const void *buf, size_t size, char *out_str, int flags)
{
	static const char convstd[64+1] =	// Standard encoding
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
	static const char convsafe[64+1] =	// URL-safe encoding
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789-_";
	const char *conv = (flags & SST_BASE64_URLSAFE) ? convsafe : convstd;

	// Handle complete 24-bit groups
	const uint8_t *b = (const uint8_t*)buf;
	char *s = out_str;
	while (size >= 3) {
		uint32_t v =	((uint32_t)b[0] << 16) |
				((uint32_t)b[1] << 8) |
				((uint32_t)b[2] << 0);
		s[0] = conv[(v >> 18) & 0x3f];
		s[1] = conv[(v >> 12) & 0x3f];
		s[2] = conv[(v >> 6) & 0x3f];
		s[3] = conv[(v >> 0) & 0x3f];
		size -= 3;
		b += 3;
		s += 4;
	}

	if (size == 0) {

		// Data was an even number of 24-bit groups in size.
		*s = 0;
		return s - out_str;

	} else {

		// Handle the final partial group
		char pad = (flags & SST_BASE64_NOPAD) ? 0 : '=';
		uint32_t v = (uint32_t)(*b++) << 16;
		s[4] = 0;
		s[3] = pad;
		if (--size) {
			v |= (uint32_t)(*b++) << 8;
			s[2] = conv[(v >> 6) & 0x3f];
		} else {
			s[2] = pad;
		}
		s[1] = conv[(v >> 12) & 0x3f];
		s[0] = conv[(v >> 18) & 0x3f];
		return (s - out_str) + strlen(s);
	}
}

int sst_base64decode(const char *str, void *out_buf, size_t *inout_size)
{
	const char *s = str;
	uint8_t *b = (uint8_t*)out_buf;
	uint8_t *bmax = b + *inout_size;

#define DECODE(c)	\
	c = *s++;	\
	c = dectab[(uint8_t)c];	\
	if (c < 0) break;

#define OUTPUT(v) \
	if (b == bmax) { errno = EINVAL; return -1; }	\
	*b++ = (v);

	while (1) {
		signed char c;

		// Decode first binary output byte in a 24-bit group
		DECODE(c);
		uint32_t v = (uint32_t)c << 18;
		DECODE(c);
		v |= (uint32_t)c << 12;
		OUTPUT(v >> 16);

		// Second output byte
		DECODE(c);
		v |= (uint32_t)c << 6;
		OUTPUT(v >> 8);

		// Third output byte
		DECODE(c);
		v |= (uint32_t)c;
		OUTPUT(v);
	}

	// Encountered terminator or illegal character.
	// Return actual amount of input string and buffer space consumed.
	*inout_size = b - (uint8_t*)out_buf;
	return s - str;
}

