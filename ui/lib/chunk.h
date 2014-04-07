// Basic format defs - should be moved into a C-only header
#define VXA_ARCHSIG 0x56584168  // 'VXAh'
#define VXA_CHUNKSIG    0x56584163  // 'VXAc'

#define VXA_IHASH_SIZE  32  // Size of inner hash (encrypt key)
#define VXA_OHASH_SIZE  64  // Size of outer hash
#define VXA_OCHECK_SIZE 8   // Size of outer hash check
#define VXA_RECOG_SIZE  32  // Size of recognizer preamble


/**
 * A 64-bit outer hash check is simply the first 8 bytes of a chunk's 512-bit outer hash.
 */
typedef quint64 OuterCheck;


/**
 * Extract the 64-bit check field from a full outer hash.
 * @param  ohash full 512-bit outer hash
 * @return       64-bit prefix
 */
OuterCheck hashCheck(const QByteArray &ohash);
