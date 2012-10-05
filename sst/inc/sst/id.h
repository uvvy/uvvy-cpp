#ifndef SST_C_ID_H
#define SST_C_ID_H

#include <sys/socket.h>

#include <sst/decls.h>
#include <sst/base64.h>

SST_BEGIN_DECLS


/** An sst_ident represents an SST endpoint identifier (EID).
 * SST uses EIDs in place of IP addresses to identify hosts
 * or virtual endpoint identities on a particular host
 * (e.g., identites for specific user accounts on multiuser hosts).
 * An EID is a variable-length binary string of bytes,
 * whose exact interpretation depends on the scheme number
 * embedded in the first 6 bits of each EID.
 * EIDs can represent both cryptographically self-certifying identifiers
 * and legacy addresses such as IP addresses and IP/port pairs.
 * Although EIDs are not usually intended to be seen by the user,
 * they have a standard filename/URL-compatible base64 text encoding,
 * in which the first character encodes the scheme number.
 */
typedef unsigned char sst_id;


/** Constant representing a reasonable size limit for EIDs in the near future.
 * This value is not a permanent upper limit and may eventually increase! */
#define SST_IDMAXLEN	256


/** Endpoint identifier scheme numbers.
 * The scheme number occupies the top 6 bits in any EID,
 * making the EID's scheme easily recognizable
 * via the first character in its Base64 representation. */
enum sst_idscheme {
	sst_id_none	= 0,	///< Reserved for the "null" Ident.

	// Non-cryptographic legacy address schemes
	sst_id_mac	= 1,	///< IEEE MAC address
	sst_id_ip	= 2,	///< IP address with optional port

	// Cryptographically self-certifying identity schemes
	sst_id_dsa160	= 10,	///< DSA with SHA-256, yielding 160-bit IDs
	sst_id_rsa160	= 11,	///< RSA with SHA-256, yielding 160-bit IDs
};


/** Read the scheme number from an EID. */
static inline sst_idscheme sst_id_scheme(const sst_id *id)
{
	return (sst_idscheme)(id[0] >> 2);
}

/** Create an EID representing a non-cryptographic IP address
 * with optional TCP or UDP port number.
 * @param sa a sockaddr structure containing the IP address and port number.
 *		If the port number in the sockaddr struct is zero,
 *		no port number is included in the EID.
 * @param idbuf the buffer in which to produce the EID.
 *		The application should provide a buffer at least SST_ID_LEN
 * @param buflen the size of the output buffer.
 * @return the actual length of the generated EID on success,
 *	or -1 with errno set on error: for example,
 *	EAFNOSUPPORT if the provided sockaddr's address family is unknown,
 *	EINVAL if the buffer is not large enough for the resulting EID.
 */
int sst_idfromsockaddr(const struct sockaddr *sa, sst_id *idbuf, int buflen);

/** Extract an EID containing an IP endpoint into a sockaddr structure.
 * @param idbuf the EID to extract.
 * @param idlen the length in bytes of the EID.
 * @param sa the sockaddr structure to fill in.
 * @param salen the size of the sockaddr buffer to be filled.
 * @return 0 on success, or -1 with errno set on error: for example,
 *	EINVAL if the EID is invalid or uses a scheme
 *	that cannot be extracted into a sockaddr structure,
 *	or if the output buffer is too small.
 */
int sst_idtosockaddr(const sst_id *idbuf, int idlen,
			struct sockaddr *sa, int salen);


/** Convert a binary EID to SST's standard base64 encoding.
 * SST's standard encoding is the URL-safe encoding defined by RFC 3548,
 * except omitting any '=' pad characters at the end.
 * @param idbuf the EID to base64-encode.
 * @param idsize the size of the EID in bytes.
 * @param out_str the buffer in which to write the encoded EID.
 *	The output string buffer must be big enough for the encoded string,
 *	as computed by the SST_BASE64ENCSPACE macro in sst/base64.h.
 * @return the length of the output string not including terminating NULL. */
static inline int sst_idencode(const sst_id *idbuf, size_t idsize,
				char *out_str)
{
	return sst_base64encode(idbuf, idsize, out_str,
				SST_BASE64_URLSAFE | SST_BASE64_NOPAD);
}

/** Decode a base64-encoded EID into a binary byte string.
 * On entry, *inout_size contains the maximum size of the output buffer.
 * Decodes until a terminating null or invalid character is seen in the input,
 * or until the output buffer is exhausted.
 * In the former case, returns the number of characters successfully read,
 * and sets *inout_size to the actual number of binary bytes produced.
 * In the latter case, returns -1 with errno = EINVAL.  */
static inline int sst_iddecode(const char *str,
				sst_id *outid, size_t *inout_idsize)
{
	return sst_base64decode(str, outid, inout_idsize);
}


SST_END_DECLS

#endif	// SST_C_ID_H
