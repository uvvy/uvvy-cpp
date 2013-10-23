# Some interesting quotations from other projects

AB[C](#c)DEF[G](#g)HIJKLM[N](#n)OPQRSTUVWXYZ

## C

### CCNx

> http://www.ccnx.org/

### Cryptosphere

> [notes on security and crypto](https://github.com/cryptosphere/cryptosphere/wiki/Protocol) - CurveCP and DTLS

> [CS trello taskboard](https://trello.com/b/WMKsvLOW/cryptosphere)

## G

### [GNUnet](https://gnunet.org)

> Anonymity is provided by making messages originating from a peer indistinguishable from messages that the peer is routing. All peers act as routers and use link-encrypted connections with stable bandwidth utilization to communicate with each other.

* The purpose of this service is to manage private keys that
* represent the various egos/pseudonyms/identities of a GNUnet user.

> Pseudonyms in GNUnet are essentially public-private (RSA) key pairs that allow a GNUnet user to maintain an identity (which may or may not be detached from his real-life identity). GNUnet's pseudonyms are not file-sharing specific --- and they will likely be used by many GNUnet applications where a user identity is required.

> Note that a pseudonym is NOT bound to a GNUnet peer. There can be multiple pseudonyms for a single user, and users could (theoretically) share the private pseudonym keys (currently only out-of-band by knowing which files to copy around).

> *MN note*: UI representation: similar to Urbit, but combine all sections together, e.g. have a subsection
for lord/lady, a subsection for punk, subsection for anon, subsection for org and so on.

* GNUnet itself never interprets the contents of shared files, except when using GNU libextractor to obtain keywords.

> Depending on the peer's configuration, GNUnet peers migrate content between peers. Content in this sense are individual blocks of a file, not necessarily entire files. When peers run out of space (due to local publishing operations or due to migration of content from other peers), blocks sometimes need to be discarded. GNUnet first always discards expired blocks (typically, blocks are published with an expiration of about two years in the future; this is another option). If there is still not enough space, GNUnet discards the blocks with the lowest priority. The priority of a block is decided by its popularity (in terms of requests from peers we trust) and, in case of blocks published locally, the base-priority that was specified by the user when the block was published initially.

> When peers migrate content to other systems, the replication level of a block is used to decide which blocks need to be migrated most urgently. GNUnet will always push the block with the highest replication level into the network, and then decrement the replication level by one. If all blocks reach replication level zero, the selection is simply random.

> https://gnunet.org/gns

> You can usually get the hash of your public key using blablabla. Alternatively, you can obtain a QR code with your zone key AND your pseudonym from gnunet-setup. The QR code is displayed in the GNS tab and can be stored to disk using the Save as button next to the image.

## N

### NaCl

> NaCl provides a simple crypto_box function that does everything in one step. The function takes the sender's secret key, the recipient's public key, and a message, and produces an authenticated ciphertext. All objects are represented in wire format, as sequences of bytes suitable for transmission; the crypto_box function automatically handles all necessary conversions, initializations, etc.

> All of the NaCl software is in the public domain.

Not CC0, but probably bearable.

[cmakeified version](https://github.com/cjdelisle/cnacl)

cryptobox functions are handy (although throwing exceptions is a bit excess), also this may remove
dependency on whole openssl. nacl should be much more manageable.

