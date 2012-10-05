//
// UIP uses the "URL and Filename Safe" Base 64 encoding scheme
// specified in RFC 3548 to represent pseudorandom identifiers
// such as endpoint identifiers and chunk hashes in printable form.
// The only difference from RFC 3548 is that UIP does not use
// the '=' character to pad odd-length trailing character groups.
//
#ifndef UIP_BASE64_H
#define UIP_BASE64_H

#include <sys/types.h>


/* Compute the amount of string buffer space required,
 * to base-64 encode a binary data buffer of a given length,
 * including the terminating null. */
#define UIP_BASE64ENCSPACE(size)	(((size) * 4 + 5) / 3)

/* Compute the maximum amount of binary data buffer space required
 * to decode a base-64 encoded string of a given length. */
#define UIP_BASE64DECSPACE(len)		(((len) * 3) / 4)


/* Encode the contents of a binary data buffer into UIP's base64 notation.
 * The output string buffer must be big enough for the encoded string,
 * as computed by the UIP_BASE64DECSPACE macro above.
 * Returns the length of the output string not including terminating NULL. */
int uip_base64encode(const void *buf, size_t size, char *out_str);

/* Decode a base32-encoded string into a binary buffer.
 * On entry, *inout_size contains the maximum size of the output buffer.
 * Decodes until a terminating null or invalid character is seen in the input,
 * or until the output buffer is exhausted.
 * In the former case, returns the number of characters successfully read,
 * and sets *inout_size to the actual number of binary bytes produced.
 * In the latter case, returns -1 with errno = EINVAL.  */
int uip_base64decode(const char *str, void *out_buf, size_t *inout_size);


#endif	/* UIP_BASE32_H */
