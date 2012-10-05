//
// $Id: base64.h 798 2006-03-29 16:37:37Z baford $
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
//
// UIA uses the "URL and Filename Safe" Base 64 encoding scheme
// specified in RFC 3548 to represent pseudorandom identifiers
// such as endpoint identifiers and chunk hashes in printable form.
// The only difference from RFC 3548 is that UIA does not use
// the '=' character to pad odd-length trailing character groups.
//
#ifndef SST_C_BASE64_H
#define SST_C_BASE64_H

#include <sys/types.h>

#include <sst/decls.h>

SST_BEGIN_DECLS


/** Compute the amount of string buffer space required,
 * to base-64 encode a binary data buffer of a given length,
 * including the terminating null. */
#define SST_BASE64ENCSPACE(size)	((((size) + 2) / 3) * 4 + 1)

/** Compute the maximum amount of binary data buffer space required
 * to decode a base-64 encoded string of a given length. */
#define SST_BASE64DECSPACE(len)		(((len) * 3) / 4)


/* Flags controlling base64 encoding options. */
#define SST_BASE64_URLSAFE	0x01	// Use URL-safe encoding
#define SST_BASE64_NOPAD	0x02	// Don't write '=' pad characters


/** Encode the contents of a binary data buffer into base64 notation.
 * The output string buffer must be big enough for the encoded string,
 * as computed by the SST_BASE64ENCSPACE macro above.
 * Returns the length of the output string not including terminating NULL. */
int sst_base64encode(const void *buf, size_t size, char *out_str, int flags);

/* Decode a base64-encoded string into a binary buffer.
 * On entry, *inout_size contains the maximum size of the output buffer.
 * Decodes until a terminating null or invalid character is seen in the input,
 * or until the output buffer is exhausted.
 * In the former case, returns the number of characters successfully read,
 * and sets *inout_size to the actual number of binary bytes produced.
 * In the latter case, returns -1 with errno = EINVAL.  */
int sst_base64decode(const char *str, void *out_buf, size_t *inout_size);


SST_END_DECLS

#endif	/* SST_C_BASE64_H */
