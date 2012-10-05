// XDR definitions for SST's key negotiation protocol


// Length-delimited option fields used in key negotiation chunks
enum KeyOptionType {
	KeyOptionInvalid	= 0
};
union KeyOptionUnion switch (KeyOptionType type) {
	case KeyOptionInvalid:	void;
};
typedef KeyOptionUnion ?KeyOption;


////////// Lightweight Checksum Negotiation //////////

// Checksum negotiation chunks
struct KeyChunkChkI1Data {
	// XXX nonces should be 64-bit, to ensure USIDs unique over all time!
	unsigned int	cki;		// Initiator's checksum key
	unsigned char	chani;		// Initiator's channel number
	opaque		cookie<>;	// Responder's cookie, if any
	opaque		ulpi<>;		// Upper-level protocol data
	opaque		cpkt<>;		// Piggybacked channel packet
};
struct KeyChunkChkR1Data {
	unsigned int	cki;		// Initiator's checksum key, echoed
	unsigned int	ckr;		// Responder's checksum key,
					// = 0 if cookie required
	unsigned char	chanr;		// Responder's channel number,
					// 0 if cookie required
	opaque		cookie<>;	// Responder's cookie, if any
	opaque		ulpr<>;		// Upper-level protocol data
	opaque		cpkt<>;		// Piggybacked channel packet
};


////////// Diffie-Helman Key Negotiation //////////

// DH groups the initiator may select for DH/JFK negotiation
enum DhGroup {
	DhGroup1024	= 0x01,	// 1024-bit DH group
	DhGroup2048	= 0x02,	// 2048-bit DH group
	DhGroup3072	= 0x03	// 3072-bit DH group
};

// DH/JFK negotiation chunks
struct KeyChunkDhI1Data {
	DhGroup		group;		// DH group of initiator's public key
	int		keymin;		// Minimum AES key length: 16, 24, 32
	opaque		nhi[32];	// Initiator's SHA256-hashed nonce
	opaque		dhi<384>;	// Initiator's DH public key
	opaque		eidr<256>;	// Optional: desired EID of responder
};
struct KeyChunkDhR1Data {
	DhGroup		group;		// DH group for public keys
	int		keylen;		// Chosen AES key length: 16, 24, 32
	opaque		nhi[32];	// Initiator's hashed nonce
	opaque		nr[32];		// Responder's nonce
	opaque		dhr<384>;	// Responder's DH public key
	opaque		hhkr<256>;	// Responder's challenge cookie
	opaque		eidr<256>;	// Optional: responder's EID
	opaque		pkr<>;		// Optional: responder's public key
	opaque		sr<>;		// Optional: responder's signature
};
struct KeyChunkDhI2Data {
	DhGroup		group;		// DH group for public keys
	int		keylen;		// AES key length: 16, 24, or 32
	opaque		ni[32];		// Initiator's original nonce
	opaque		nr[32];		// Responder's nonce
	opaque		dhi<384>;	// Initiator's DH public key
	opaque		dhr<384>;	// Responder's DH public key
	opaque		hhkr<256>;	// Responder's challenge cookie
	opaque		idi<>;		// Initiator's encrypted identity
};
struct KeyChunkDhR2Data {
	opaque		nhi[32];	// Initiator's original nonce
	opaque		idr<>;		// Responder's encrypted identity
};


// Encrypted and authenticated identity blocks for I2 and R2 messages
struct KeyIdentI {
	unsigned char	chani;		// Initiator's channel number
	opaque		eidi<256>;	// Initiator's endpoint identifier
	opaque		eidr<256>;	// Desired EID of responder
	opaque		idpki<>;	// Initiator's identity public key
	opaque		sigi<>;		// Initiator's parameter signature
	opaque		ulpi<>;		// Upper-level protocol data
};
struct KeyIdentR {
	unsigned char	chanr;		// Responder's channel number
	opaque		eidr<256>;	// Responder's endpoint identifier
	opaque		idpkr<>;	// Responder's identity public key
	opaque		sigr<>;		// Responder's parameter signature
	opaque		ulpr<>;		// Upper-level protocol data
};

// Responder DH key signing block for R2 messages (JFKi only)
struct KeyParamR {
	DhGroup		group;		// DH group for public keys
	opaque		dhr<384>;	// Responder's DH public key
};

// Key parameter signing block for I2 and R2 messages
struct KeyParams {
	DhGroup		group;		// DH group for public keys
	int		keylen;		// AES key length: 16, 24, or 32
	opaque		nhi[32];	// Initiator's hashed nonce
	opaque		nr[32];		// Responder's nonce
	opaque		dhi<384>;	// Initiator's DH public key
	opaque		dhr<384>;	// Responder's DH public key
	opaque		eid<256>;	// Peer's endpoint identifier
};


enum KeyChunkType {
	// Generic chunks used by multiple negotiation protocols
	KeyChunkPacket	= 0x0001,	// Piggybacked packet for new channel

	// Lightweight checksum negotiation
	KeyChunkChkI1	= 0x0011,
	KeyChunkChkR1	= 0x0012,

	// Diffie-Hellman key agreement using JFK protocol
	KeyChunkDhI1	= 0x0021,
	KeyChunkDhR1	= 0x0022,
	KeyChunkDhI2	= 0x0023,
	KeyChunkDhR2	= 0x0024
};
union KeyChunkUnion switch (KeyChunkType type) {
	case KeyChunkPacket:	opaque packet<>;

	case KeyChunkChkI1:	KeyChunkChkI1Data chki1;
	case KeyChunkChkR1:	KeyChunkChkR1Data chkr1;

	case KeyChunkDhI1:	KeyChunkDhI1Data dhi1;
	case KeyChunkDhR1:	KeyChunkDhR1Data dhr1;
	case KeyChunkDhI2:	KeyChunkDhI2Data dhi2;
	case KeyChunkDhR2:	KeyChunkDhR2Data dhr2;
};
typedef KeyChunkUnion ?KeyChunk;


// Top-level format of all negotiation protocol messages
struct KeyMessage {
	int		magic;		// 24-bit magic value
	KeyChunk	chunks<>;	// Message chunks
};


