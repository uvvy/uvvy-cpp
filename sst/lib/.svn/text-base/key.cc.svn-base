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


// XXX bug in g++ 4.1.2?  This must be declared before including QHash!?
#include <QtGlobal>
namespace SST { class Endpoint; }
uint qHash(const SST::Endpoint &ep);

#include <QtDebug>

#include "key.h"
#include "keyproto.h"
#include "sock.h"
#include "flow.h"
#include "util.h"
#include "xdr.h"
#include "ident.h"
#include "host.h"
#include "dh.h"		// XX support multiple cyphersuites
#include "aes.h"
#include "hmac.h"

#include <openssl/dsa.h>
#include <openssl/rand.h>

using namespace SST;


#define NONCELEN	SHA256_DIGEST_LENGTH



////////// Checksum security setup //////////

// Calculate a checksum key for TCP-grade "security".
// Uses an analog of Bellovin's RFC 1948 for keying TCP sequence numbers.
// We use our own host identity, including private key,
// as the host-specific secret data to drive the key generation.
// XX would probably be better to use some unrelated persistent random bits.
static quint32 calcChkKey(Host *h, quint8 chanid, QByteArray peerid)
{
	Ident hostid = h->hostIdent();
	Q_ASSERT(!hostid.isNull());
	Q_ASSERT(hostid.havePrivateKey());
	QByteArray hostkey = hostid.key(true);
	QByteArray hosthash = Sha256::hash(hostkey);
	Q_ASSERT(hosthash.size() == HMACKEYLEN);

	// Compute a keyed hash of the local channel ID and peer's host ID
	hmac_ctx ctx;
	hmac_init(&ctx, (const uint8_t*)hosthash.data());
	hmac_update(&ctx, &chanid, 1);
	hmac_update(&ctx, peerid.data(), peerid.size());
	quint32 ck;
	hmac_final(&ctx, (const uint8_t*)hosthash.data(),
			(uint8_t*)&ck, sizeof(ck));

	// Finally, add the current system time in 4-microsecond increments,
	// to ensure that packets for old channels cannot accidentally
	// be accepted into new channels within a 4.55-hour window.
	return ck + h->currentTime().usecs / 4;
}

// Calculate a channel ID from the local and remote checksum keys.
// The two checksum keys must be different!
// (Otherwise we'd get the same channel ID in both directions.)
static QByteArray calcChkChanId(quint32 localck, quint32 remoteck)
{
	Q_ASSERT(localck != remoteck);
	quint32 buf[2] = { htonl(localck), htonl(remoteck) };
	return QByteArray((char*)&buf, sizeof(buf));
}


////////// Cryptographic security setup //////////

static QByteArray calcSigHash(DhGroup group, int keylen,
		const QByteArray &nhi, const QByteArray &nr,
		const QByteArray &dhi, const QByteArray &dhr,
		const QByteArray &eid)
{
	// Build the parameter block
	KeyParams kp;
	kp.group = group;
	kp.keylen = keylen;
	kp.nhi = nhi;
	kp.nr = nr;
	kp.dhi = dhi;
	kp.dhr = dhr;
	kp.eid = eid;

	// Compute the message to sign
	QByteArray ba;
	XdrStream wds(&ba, QIODevice::WriteOnly);
	wds << kp;
	Q_ASSERT(wds.status() == wds.Ok);

	// Return its digest
	return Sha256::hash(ba);
}

static QByteArray calcKey(const QByteArray &master,
		const QByteArray &nhi, const QByteArray &nr,
		char which, int keylen)
{
	//qDebug("calcKey: master size %d bytes", master.size());

	QByteArray masterHash = Sha256::hash(master);
	Q_ASSERT(masterHash.size() == HMACKEYLEN);

	hmac_ctx ctx;
	hmac_init(&ctx, (const uint8_t*)masterHash.data());
	hmac_update(&ctx, nhi.data(), nhi.size());
	hmac_update(&ctx, nr.data(), nr.size());
	hmac_update(&ctx, &which, 1);

	QByteArray key;
	key.resize(keylen);
	hmac_final(&ctx, (const uint8_t*)masterHash.data(),
			(uint8_t*)key.data(), keylen);
	return key;
}

static QByteArray send(quint32 magic, KeyMessage &msg,
			const SocketEndpoint &dst)
{
	msg.magic = magic;
	QByteArray buf;
	XdrStream wds(&buf, QIODevice::WriteOnly);
	wds << msg;
	Q_ASSERT(wds.status() == wds.Ok);
	dst.send(buf);
	return buf;
}

// Send a one-chunk KeyMessage
static QByteArray send(quint32 magic, KeyChunk &ch, const SocketEndpoint &dst)
{
	KeyMessage msg;
	msg.chunks.append(ch);
	return send(magic, msg, dst);
}

static uint qHash(const KeyEpChk &ch)
{
	return qHash(ch.first) + qHash(ch.second);
}


// XXX workaround for G++ template instantiation bug???
XdrStream &operator<<(XdrStream &xs, const XdrOption<KeyChunkUnion> &o)
{
	if (!o.isNull()) {
		QByteArray buf;
		XdrStream bxs(&buf, QIODevice::WriteOnly);
		bxs << *o;
		xs << buf;
	} else
		xs << o.getEncoded();
	return xs;
}
XdrStream &operator>>(XdrStream &xs, XdrOption<KeyChunkUnion> &o)
{
	QByteArray buf;
	xs >> buf;
	if (buf.isEmpty()) {
		o.clear();
		return xs;
	}
	XdrStream bxs(&buf, QIODevice::ReadOnly);
	bxs >> o.alloc();
	if (bxs.status() != bxs.Ok)
		o.setEncoded(buf);
	return xs;
}




////////// KeyResponder //////////

KeyResponder::KeyResponder(Host *host, quint32 magic, QObject *parent)
:	SocketReceiver(host, magic, parent),
	h(host)
{
}

void KeyResponder::receive(QByteArray &pkt, XdrStream &,
				const SocketEndpoint &src)
{
	// Decode the received message
	XdrStream rs(&pkt, QIODevice::ReadOnly);
	KeyMessage msg;
	rs >> msg;
	if (rs.status() != rs.Ok)
		return qDebug("Received malformed key agreement packet");

	// Find and process the first recognized primary chunk.
	for (int i = 0; i < msg.chunks.size(); i++) {
		KeyChunk &ch = msg.chunks[i];
		if (!ch) continue;	// Unrecognized chunk - not decoded
		switch (ch->type) {

		// Lightweight checksum negotiation
		case KeyChunkChkI1:
			return gotChkI1(ch->chki1, src);
		case KeyChunkChkR1:
			return KeyInitiator::gotChkR1(h, ch->chkr1, src);

		// Diffie-Hellman key negotiation
		case KeyChunkDhI1:
			return gotDhI1(ch->dhi1, src);
		case KeyChunkDhI2:
			return gotDhI2(ch->dhi2, src);
		case KeyChunkDhR1:
			return KeyInitiator::gotDhR1(h, ch->dhr1);
		case KeyChunkDhR2:
			return KeyInitiator::gotDhR2(h, ch->dhr2);

		default: break;	// ignore other chunk types
		}
	}

	// If there were no recognized primary chunks,
	// it might be just a responder's ping packet for hole punching.
	return KeyInitiator::gotR0(h, src);
}

void KeyResponder::sendR0(const Endpoint &dst)
{
	qDebug() << this << "send R0 to" << dst.toString();
	KeyMessage msg;
	foreach (Socket *sock, h->activeSockets()) {
		SocketEndpoint sdst(dst, sock);
		send(magic(), msg, sdst);
	}
}

void KeyResponder::gotChkI1(KeyChunkChkI1Data &i1, const SocketEndpoint &src)
{
	qDebug() << this << "got ChkI1 from" << src.toString();

	if (i1.chani == 0)
		return;		// Invalid initiator channel number

	// XXX implement DoS protection via cookies

	// Create a legacy EID to represent the responder's (weak) identity
	Ident identi = Ident::fromIpAddress(src.addr, src.port);
	QByteArray eidi = identi.id();

	// Check for duplicate, already-accepted I1 requests
	QByteArray armorid = eidi;
	armorid.append(QByteArray((char*)&i1.cki, sizeof(i1.cki)));
	armorid.append(QByteArray((char*)&i1.chani, sizeof(i1.chani)));
	qDebug() << "armorid" << armorid.toHex();
	if (chkflows.value(armorid) != NULL) {
		qDebug("Dropping duplicate ChkI1 for already-setup flow");
		return;
	}

	// Check that the initiator is someone we want to talk with!
	if (!checkInitiator(src, eidi, i1.ulpi)) {
		qDebug("Rejecting ChkI1 due to checkInitiator()");
		return;	// XXX generate cached error response instead
	}

	// Setup a flow and produce our R1 response.
	QByteArray ulpr;
	Flow *flow = newFlow(src, eidi, i1.ulpi, ulpr);
	if (!flow) {
		qDebug("Rejecting ChkI1 due to NULL return from newFlow()");
		return;	// XXX generate cached error response instead
	}
	Q_ASSERT(flow->isBound());
	Q_ASSERT(!flow->isActive());

	// Compute a checksum key for our end
	quint32 ckr = calcChkKey(h, flow->localChannel(), eidi);
	if (ckr == i1.cki)
		ckr++;	// Make sure it's different from cki!

	// Should be no failures after this point.
	ChecksumArmor *armor = new ChecksumArmor(ckr, i1.cki, armorid);
	flow->setArmor(armor);

	// Keep track of active flows to detect duplicates of this ChkI1
	chkflows.insert(armorid, armor);
	connect(armor, SIGNAL(destroyed(QObject*)),
		this, SLOT(checksumArmorDestroyed(QObject*)));

	// Set the channel IDs for the new stream.
	QByteArray txchanid = calcChkChanId(ckr, i1.cki);
	QByteArray rxchanid = calcChkChanId(i1.cki, ckr);
	flow->setChannelIds(txchanid, rxchanid);

	// Build, send, XXX and cache our R1 response.
	KeyChunk ch;
	KeyChunkUnion &chu = ch.alloc();
	chu.type = KeyChunkChkR1;
	chu.chkr1.cki = i1.cki;
	chu.chkr1.ckr = ckr;
	chu.chkr1.chanr = flow->localChannel();
	chu.chkr1.ulpr = ulpr;
	QByteArray r2pkt = send(magic(), ch, src);
	// XXX hk->r2cache.insert(hhkr, r2pkt);

	// Let the ball roll
	flow->setRemoteChannel(i1.chani);
	flow->start(false);
}

void KeyResponder::checksumArmorDestroyed(QObject *obj)
{
	ChecksumArmor *armor = (ChecksumArmor*)obj;
	qDebug() << this << "checksumArmorDestroyed" << armor;

	int n = chkflows.remove(armor->id());
	Q_ASSERT(n == 1);
}


QByteArray KeyResponder::calcDhCookie(DHKey *hk,
		const QByteArray &nr, const QByteArray &nhi,
		const Endpoint &src)
{
	// Put together the data to hash
	QByteArray buf;
	XdrStream wds(&buf, QIODevice::WriteOnly);
	wds << hk->pubkey << nr << nhi << src.addr.toIPv4Address() << src.port;
	Q_ASSERT(wds.status() == wds.Ok);

	// Compute the keyed hash
	Q_ASSERT(sizeof(hk->hkr) == HMACKEYLEN);
	hmac_ctx ctx;
	hmac_init(&ctx, hk->hkr);
	hmac_update(&ctx, buf.data(), buf.size());
	QByteArray hhkr;
	hhkr.resize(HMACLEN);
	hmac_final(&ctx, hk->hkr, (quint8*)hhkr.data(), HMACLEN);

	return hhkr;
}

void KeyResponder::gotDhI1(KeyChunkDhI1Data &i1, const SocketEndpoint &src)
{
	qDebug() << this << "got DhI1 from" << src.addr.toString() << src.port;

	// Find or generate the appropriate host key
	DHKey *hk = h->getDHKey(i1.group);
	if (hk == NULL)
		return;		// Unrecognized DH group
	if (i1.dhi.size() > DH_size(hk->dh))
		return;		// Public key too large
	if (i1.keymin != 128/8 && i1.keymin != 192/8 && i1.keymin != 256/8)
		return;		// Invalid minimum AES key length

	// Generate an unpredictable responder's nonce
	QByteArray nr = randBytes(NONCELEN);

	// Compute the hash challenge
	QByteArray hhkr = calcDhCookie(hk, nr, i1.nhi, src);

	// Build and send the response
	KeyChunk ch;
	KeyChunkUnion &chu = ch.alloc();
	chu.type = KeyChunkDhR1;
	chu.dhr1.group = i1.group;
	chu.dhr1.keylen = i1.keymin;
	chu.dhr1.nhi = i1.nhi;
	chu.dhr1.nr = nr;
	chu.dhr1.dhr = hk->pubkey;
	chu.dhr1.hhkr = hhkr;
	// Don't offer responder's identity for now
	send(magic(), ch, src);
}

void KeyResponder::gotDhI2(KeyChunkDhI2Data &i2, const SocketEndpoint &src)
{
	qDebug() << this << "got DhI2";

	// We'll need the originator's hashed nonce as well...
	QByteArray nhi = Sha256::hash(i2.ni);

	// Find the appropriate host key
	DHKey *hk = h->getDHKey(i2.group);
	if (hk == NULL || i2.dhr != hk->pubkey) {
		// Key mismatch, probably due to a timeout and key change.
		// Send a new R1 response instead of an R2.
		qDebug("Received I2 packet with incorrect public key");
		KeyChunkDhI1Data i1;
		i1.group = i2.group;
		i1.keymin = i2.keylen;
		i1.nhi = nhi;
		i1.dhi = i2.dhi;
		return gotDhI1(i1, src);
	}

	// See if we've already responded to this particular I2 -
	// if so, just return our previous cached response.
	// Use hhkr as the index, as per the JFK spec.
	if (hk->r2cache.contains(i2.hhkr)) {
		qDebug("Received duplicate I2 packet");
		src.send(hk->r2cache[i2.hhkr]);
		return;
	}

	// Verify the challenge hash
	if (i2.hhkr != calcDhCookie(hk, i2.nr, nhi, src)) {
		qDebug("Received I2 with bad challenge hash");
		return;		// Just drop the bad I2
	}

	// Compute the shared master secret
	QByteArray master = hk->calcKey(i2.dhi);

	// Check and strip the MAC field on the encrypted identity
	QByteArray mackey = calcKey(master, nhi, i2.nr, '2', 256/8);
	if (!HMAC(mackey).calcVerify(i2.idi)) {
		qDebug("Received I2 with bad initiator identity MAC");
		return;	// XXX generate cached error response instead
	}

	// Decrypt it with AES-256-CBC
	QByteArray enckey = calcKey(master, nhi, i2.nr, '1', 256/8);
	i2.idi = AES().setDecryptKey(enckey).cbcDecrypt(i2.idi);

	// Decode the identity information
	XdrStream encrds(i2.idi);
	KeyIdentI kii;
	encrds >> kii;
	if (encrds.status() != encrds.Ok || kii.chani == 0) {
		qDebug("Received I2 with bad identity info");
		return;	// XXX generate cached error response instead
	}

	// Check that the initiator is someone we want to talk with!
	if (!checkInitiator(src, kii.eidi, kii.ulpi)) {
		qDebug("Rejecting I2 due to checkInitiator()");
		return;	// XXX generate cached error response instead
	}

	// Check that the initiator actually wants to talk with us
	Ident hi = h->hostIdent();
	QByteArray eidr = kii.eidr;
	QByteArray hid = hi.id();
	if (eidr.isEmpty()) {
		eidr = hid;
	} else if (eidr != hid) {
		qDebug("Received I2 from initiator looking for someone else");
		return;	// XXX generate cached error response instead
	}

	// Verify the initiator's identity
	//qDebug() << "eidi" << kii.eidi.toBase64()
	//	<< "idpki" << kii.idpki.toBase64()
	//	<< "sigi" << kii.sigi.toBase64();
	Ident identi(kii.eidi);
	if (!identi.setKey(kii.idpki)) {
		qDebug("Received I2 with bad initiator public key");
		return;	// XXX generate cached error response instead
	}
	QByteArray sighash = calcSigHash(i2.group, i2.keylen, nhi, i2.nr,
					i2.dhi, i2.dhr, kii.eidr);
	//qDebug("idi %s\nidpki %s\nsighash %s\nsigi %s\n",
	//	idi.toBase64().data(), idpki.toBase64().data(),
	//	sighash.toBase64().data(), sigi.toBase64().data());
	if (!identi.verify(sighash, kii.sigi)) {
		qDebug("Received I2 with bad initiator signature");
		return;	// XXX generate cached error response instead
	}

	//qDebug() << "Authenticated initiator ID" << idi.toBase64()
	//	<< "at" << src.toString();

	// Everything looks good - setup a flow and produce our R2 response.
	QByteArray ulpr;
	Flow *flow = newFlow(src, kii.eidi, kii.ulpi, ulpr);
	if (!flow) {
		qDebug("Rejecting I2 due to NULL return from newFlow()");
		return;	// XXX generate cached error response instead
	}
	Q_ASSERT(flow->isBound());
	Q_ASSERT(!flow->isActive());

	// Should be no failures after this point.

	// Sign the key parameters to prove our own identity
	sighash = calcSigHash(i2.group, i2.keylen, nhi, i2.nr,
					i2.dhi, i2.dhr, kii.eidi);
	QByteArray sigr = hi.sign(sighash);

	// Build the part of the I2 message to be encrypted.
	// (XX should we include anything for the 'sa' in the JFK spec?)
	KeyIdentR kir;
	kir.chanr = flow->localChannel();
	kir.eidr = eidr;
	kir.idpkr = hi.key();
	kir.sigr = sigr;
	kir.ulpr = ulpr;
	QByteArray encidr;
	XdrStream wds(&encidr, QIODevice::WriteOnly);
	wds << kir;
	Q_ASSERT(wds.status() == wds.Ok);
	//qDebug() << "encidr:" << encidr.toBase64() << "size" << encidr.size();

	// XX There appears to be a bug in the "optimized" x86 version
	// of AES-CBC at least in openssl-0.9.8b when given an input
	// that is not an exact multiple of the block length.
	// (The C implementation in e.g., OpenSSL 0.9.7 works fine.)
	encidr.resize((encidr.size() + 15) & ~15);

	// Encrypt and authenticate our identity
	encidr = AES().setEncryptKey(enckey).cbcEncrypt(encidr);
	HMAC(mackey).calcAppend(encidr);

	// Build, send, and cache our R2 response.
	KeyChunk ch;
	KeyChunkUnion &chu = ch.alloc();
	chu.type = KeyChunkDhR2;
	chu.dhr2.nhi = nhi;
	chu.dhr2.idr = encidr;
	QByteArray r2pkt = send(magic(), ch, src);
	hk->r2cache.insert(i2.hhkr, r2pkt);

	// Set up the armor for the new flow
	QByteArray txenckey = calcKey(master, i2.nr, nhi, 'E', 128/8);
	QByteArray txmackey = calcKey(master, i2.nr, nhi, 'A', 256/8);
	QByteArray rxenckey = calcKey(master, nhi, i2.nr, 'E', 128/8);
	QByteArray rxmackey = calcKey(master, nhi, i2.nr, 'A', 256/8);
	AESArmor *armor = new AESArmor(txenckey, txmackey, rxenckey, rxmackey);
	flow->setArmor(armor);

	// Set up the new flow's channel IDs
	QByteArray txchanid = calcKey(master, i2.nr, nhi, 'I', 128/8);
	QByteArray rxchanid = calcKey(master, nhi, i2.nr, 'I', 128/8);
	flow->setChannelIds(txchanid, rxchanid);

	// Let the ball roll
	flow->setRemoteChannel(kii.chani);
	flow->start(false);
}

bool KeyResponder::checkInitiator(const SocketEndpoint &,
				const QByteArray &, const QByteArray &)
{
	return true;
}


////////// KeyInitiator //////////

KeyInitiator::KeyInitiator(Flow *fl, quint32 magic,
			const QByteArray &idr, quint8 dhgroup)
:	h(fl->host()),
	fl(fl),
	sepr(fl->remoteEndpoint()),
	idr(idr),
	magic(magic),
	dhgroup(dhgroup ? dhgroup : KEYGROUP_JFDH_DEFAULT),
	keylen(128/8),
	state(I1),
	early(true),
	txtimer(fl->host())
{
	qDebug() << this << "initiating to" << sepr;
	Q_ASSERT(!sepr.isNull());
	Q_ASSERT(fl->isBound());
	Q_ASSERT(!fl->isActive());
	fl->setParent(this);

	// Choose the security method based on the target EID
	// XXX figure out how this really should work
	switch (Ident(idr).scheme()) {
	case Ident::DSA160:
	case Ident::RSA160:
		methods = KEYMETH_AES;
		break;
	case Ident::IP:
		methods = KEYMETH_CHK;
		break;
	default:
		qDebug("Unknown identity method %d", Ident(idr).scheme());
		state = Done;
		txtimer.stop();
		completed(false);
		return;
	}

	// Checksum key agreement state
	if (methods & KEYMETH_CHK) {
		// Compute a legacy identity for the responder
		Ident identr = Ident::fromIpAddress(sepr.addr, sepr.port);
		QByteArray eidr = identr.id();

		// Calculate an appropriate time-based checksum key,
		// making sure it's not already in use (however unlikely).
		chkkey = calcChkKey(h, fl->localChannel(), eidr);
		while (h->initchks.contains(KeyEpChk(sepr, chkkey)))
			chkkey++;
		h->initchks.insert(KeyEpChk(sepr, chkkey), this);
	}

	// DH/AES key agreement state
	if (methods & KEYMETH_AES) {
		ni = randBytes(NONCELEN);
		nhi = Sha256::hash(ni);
		Q_ASSERT(nhi.size() == NONCELEN);

		Q_ASSERT(!h->initnhis.contains(nhi));
		h->initnhis.insert(nhi, this);
	}

	// Register us as one of potentially several initiators
	// attempting to connect to this remote endpoint
	h->initeps.insert(sepr, this);

	connect(&txtimer, SIGNAL(timeout(bool)),
		this, SLOT(retransmit(bool)));

	sendI1();
	txtimer.start();
}

KeyInitiator::~KeyInitiator()
{
	qDebug() << this << "~KeyInitiator";
	cancel();
}

void
KeyInitiator::cancel()
{
	//qDebug() << this << "done initiating to" << sepr;

	if ((methods & KEYMETH_CHK) &&
			h->initchks.value(KeyEpChk(sepr, chkkey)) == this)
		h->initchks.remove(KeyEpChk(sepr, chkkey));

	if ((methods & KEYMETH_AES) &&
			h->initnhis.value(nhi) == this)
		h->initnhis.remove(nhi);

	h->initeps.remove(sepr, this);
}

void
KeyInitiator::gotR0(Host *h, const Endpoint &src)
{
	// Trigger a retransmission of the I1 packet
	// for each outstanding initiation attempt to the given target.
	foreach (KeyInitiator *i, h->initeps.values(src)) {
		if (i == NULL || i->state != I1)
			return;
		i->sendI1();
	}
}

void
KeyInitiator::sendI1()
{
	qDebug() << this << "send I1 to " << sepr.toString();
	state = I1;

	// Build a message containing an I1 chunk
	// for each of the allowed key agreement methods.
	KeyMessage msg;

	// I1 chunk for checksum security.
	if (methods & KEYMETH_CHK) {
		KeyChunk ch;
		KeyChunkUnion &chu(ch.alloc());
		chu.type = KeyChunkChkI1;
		chu.chki1.cki = chkkey;
		chu.chki1.chani = fl->localChannel();
		chu.chki1.cookie = cookie;
		chu.chki1.ulpi = ulpi;
		msg.chunks.append(ch);

		// ChkI1 might create receiver state as a result of first msg!
		early = false;
	}

	// I1 chunk for AES encryption with DH key agreement.
	if (methods & KEYMETH_AES) {

		// Clear any DH I2 state,
		// in case we're restarting at the I1 stage.
		nr.clear();
		dhr.clear();
		hhkr.clear();
		master.clear();

		// Initialize I1 state from our current host key.
		DHKey *hk = h->getDHKey(dhgroup);
		Q_ASSERT(hk != NULL);
		dhi = hk->pubkey;

		// Send the I1 message
		KeyChunk ch;
		KeyChunkUnion &chu(ch.alloc());
		chu.type = KeyChunkDhI1;
		chu.dhi1.group = (DhGroup)dhgroup;
		chu.dhi1.keymin = 128/8;
		chu.dhi1.nhi = nhi;
		chu.dhi1.dhi = dhi;
		// XX ch.dhi1.eidr? (only if the server's identity is public!)
		msg.chunks.append(ch);
	}

	Q_ASSERT(!msg.chunks.isEmpty());
	send(magic, msg, sepr);
}

void
KeyInitiator::gotChkR1(Host *h, KeyChunkChkR1Data &r1,
			const SocketEndpoint &src)
{
	qDebug() << "got ChkR1 from" << src.toString();

	// Lookup the Initiator based on the received checksum key
	KeyInitiator *i = h->initchks.value(KeyEpChk(src, r1.cki));
	if (i == NULL)
		return qDebug("Got ChkR1 for unknown I1");
	if (i->isDone())
		return qDebug("Got duplicate ChkR1 for completed initiator");
	Q_ASSERT(i->chkkey == r1.cki);
	Q_ASSERT(i->fl != NULL);

	// If responder's channel is zero, send another I1 with the cookie.
	if (r1.ckr == r1.cki || r1.chanr == 0) {
		if (r1.cookie.isEmpty())
			return qDebug("ChkR1 with no chanr and no cookie!?");
		i->cookie = r1.cookie;
		return i->sendI1();
	}

	// Set up the new flow's armor
	i->fl->setArmor(new ChecksumArmor(i->chkkey, r1.ckr));

	// Set the channel IDs for the new stream.
	QByteArray txchanid = calcChkChanId(i->chkkey, r1.ckr);
	QByteArray rxchanid = calcChkChanId(r1.ckr, i->chkkey);
	i->fl->setChannelIds(txchanid, rxchanid);

	// Finish flow setup
	i->fl->setRemoteChannel(r1.chanr);

	// Our job is done
	qDebug() << i << "key exchange completed!";
	i->state = Done;
	i->txtimer.stop();

	// Let the ball roll...
	i->fl->start(true);
	i->completed(true);
}

void
KeyInitiator::gotDhR1(Host *h, KeyChunkDhR1Data &r1)
{
	// Lookup the Initor based on the received nhi
	KeyInitiator *i = h->initnhis.value(r1.nhi);
	if (i == NULL || i->state == Done || i->dhgroup != r1.group)
		return qDebug("Got DhR1 for unknown I1");
	if (i->isDone())
		return qDebug("Got duplicate DhR1 for completed initiator");
	Q_ASSERT(i->nhi == r1.nhi);
	Q_ASSERT(i->fl != NULL);

	qDebug() << i << "got DhR1";

	// Validate the responder's group number
	if (r1.group > KEYGROUP_JFDH_MAX)
		return;		// Invalid group
	if (r1.group < i->dhgroup)
		return;		// Less than our minimum required

	// Validate the responder's specified AES key length
	if (r1.keylen != 128/8 && r1.keylen != 192/8 && r1.keylen != 256/8)
		return;		// Invalid AES key length
	if (r1.keylen < i->keylen)
		return;		// Less than our minimum required key length

	// If the group changes or our public key expires, revert to I1 phase.
	if (i->dhgroup != r1.group) {
		Q_ASSERT(i->dhgroup < r1.group);
		i->dhgroup = r1.group;
		return i->sendI1();
	}
	DHKey *hk = h->getDHKey(r1.group);
	Q_ASSERT(hk != NULL);
	if (i->dhi != hk->pubkey)
		return i->sendI1();

	// Always use the latest responder parameters received,
	// even if we've already received an R1 response.
	// XXX to be really DoS-protected from active attackers,
	// we should cache some number of the last R1 responses we get
	// until we receive a valid R2 response with the correct identity.
	i->keylen = r1.keylen;
	i->nr = r1.nr;
	i->dhr = r1.dhr;
	i->hhkr = r1.hhkr;

	// XX ignore any public responder identity in the R1 for now.

	// Compute the shared master secret
	i->master = hk->calcKey(r1.dhr);

	// Sign the key parameters to prove our identity
	Ident hi = h->hostIdent();
	QByteArray sighash =
		calcSigHash((DhGroup)i->dhgroup, i->keylen, i->nhi, i->nr,
				i->dhi, i->dhr, QByteArray()/*XX*/);
	QByteArray sigi = hi.sign(sighash);
	//qDebug("sighash %s\nsigi %s\n",
	//	sighash.toBase64().data(), sigi.toBase64().data());

	// Build the part of the I2 message to be encrypted.
	// (XX should we include anything for the 'sa' in the JFK spec?)
	KeyIdentI kii;
	kii.chani = i->fl->localChannel();
	kii.eidi = hi.id();
	kii.eidr = QByteArray(); 	// XX
	kii.idpki = hi.key();
	kii.sigi = sigi;
	kii.ulpi = i->ulpi;
	//qDebug() << "eidi" << kii.eidi.toBase64()
	//	<< "idpki" << kii.idpki.toBase64()
	//	<< "sigi" << kii.sigi.toBase64();
	QByteArray encidi;
	XdrStream wds(&encidi, QIODevice::WriteOnly);
	wds << kii;
	Q_ASSERT(wds.status() == wds.Ok);

	// XX There appears to be a bug in the "optimized" x86 version
	// of AES-CBC at least in openssl-0.9.8b when given an input
	// that is not an exact multiple of the block length.
	// (The C implementation in e.g., OpenSSL 0.9.7 works fine.)
	encidi.resize((encidi.size() + 15) & ~15);

	// Encrypt it with AES-256-CBC
	QByteArray enckey = calcKey(i->master, i->nhi, i->nr, '1', 256/8);
	encidi = AES().setEncryptKey(enckey).cbcEncrypt(encidi);

	// Authenticate it with HMAC-SHA256-128
	QByteArray mackey = calcKey(i->master, i->nhi, i->nr, '2', 256/8);
	HMAC(mackey).calcAppend(encidi);
	i->encidi = encidi;


	i->sendDhI2();
	i->txtimer.start();
}

void
KeyInitiator::sendDhI2()
{
	qDebug() << this << "send DhI2";
	state = I2;

	// Once the receiver gets this message, it'll create flow state.
	early = false;

	// Send the I2 message
	KeyChunk ch;
	KeyChunkUnion &chu = ch.alloc();
	chu.type = KeyChunkDhI2;
	chu.dhi2.group = (DhGroup)dhgroup;
	chu.dhi2.keylen = keylen;
	chu.dhi2.ni = ni;
	chu.dhi2.nr = nr;
	chu.dhi2.dhi = dhi;
	chu.dhi2.dhr = dhr;
	chu.dhi2.hhkr = hhkr;
	chu.dhi2.idi = encidi;
	send(magic, ch, sepr);
}

void
KeyInitiator::gotDhR2(Host *h, KeyChunkDhR2Data &r2)
{
	// Lookup the Initiator based on the received nhi
	KeyInitiator *i = h->initnhis.value(r2.nhi);
	if (i == NULL || i->state != I2)
		return;
	if (i->isDone())
		return qDebug("Got duplicate DhR2 for completed initiator");
	if (i->master.isEmpty())
		return;	// Haven't received an R1 response yet!
	Q_ASSERT(i->nhi == r2.nhi);
	Q_ASSERT(i->fl != NULL);

	qDebug() << i << "got DhR2";

	// Make sure our host key hasn't expired in the meantime
	// XXX but reverting here leaves the responder with a hung channel!
	DHKey *hk = h->getDHKey(i->dhgroup);
	Q_ASSERT(hk != NULL);
	if (i->dhi != hk->pubkey)
		return i->sendI1();


	// Check and strip the MAC field on the responder's encrypted identity
	QByteArray mackey = calcKey(i->master, i->nhi, i->nr, '2', 256/8);
	if (!HMAC(mackey).calcVerify(r2.idr)) {
		qDebug("Received R2 with bad responder identity MAC");
		return;
	}

	// Decrypt it with AES-256-CBC
	QByteArray enckey = calcKey(i->master, i->nhi, i->nr, '1', 256/8);
	r2.idr = AES().setDecryptKey(enckey).cbcDecrypt(r2.idr);

	// Decode the identity information
	//qDebug() << "encidr:" << r2.idr.toBase64() << "size" << r2.idr.size();
	XdrStream encrds(r2.idr);
	KeyIdentR kir;
	encrds >> kir;
	if (encrds.status() != encrds.Ok || !kir.chanr || kir.eidr.isEmpty()) {
		qDebug("Received R2 with bad responder identity info");
		return;
	}

	// Make sure the responder is who we actually wanted to talk to
	if (!i->idr.isEmpty() && kir.eidr != i->idr) {
		qDebug("Received R2 from responder with wrong identity");
		return;
	}

	// Verify the responder's identity
	Ident identr(kir.eidr);
	if (!identr.setKey(kir.idpkr)) {
		qDebug("Received R2 with bad responder public key");
		return;
	}
	QByteArray eidi = h->hostIdent().id();
	QByteArray sighash = calcSigHash((DhGroup)i->dhgroup, i->keylen,
					i->nhi, i->nr, i->dhi, i->dhr, eidi);
	if (!identr.verify(sighash, kir.sigr)) {
		qDebug("Received I2 with bad responder signature");
		return;
	}

	// Set up the new flow's armor
	QByteArray txenckey = calcKey(i->master, i->nhi, i->nr, 'E', 128/8);
	QByteArray txmackey = calcKey(i->master, i->nhi, i->nr, 'A', 256/8);
	QByteArray rxenckey = calcKey(i->master, i->nr, i->nhi, 'E', 128/8);
	QByteArray rxmackey = calcKey(i->master, i->nr, i->nhi, 'A', 256/8);
	i->fl->setArmor(new AESArmor(txenckey, txmackey, rxenckey, rxmackey));

	// Set up the new flow's channel IDs
	QByteArray txchanid = calcKey(i->master, i->nhi, i->nr, 'I', 128/8);
	QByteArray rxchanid = calcKey(i->master, i->nr, i->nhi, 'I', 128/8);
	i->fl->setChannelIds(txchanid, rxchanid);

	// Finish flow setup
	i->fl->setRemoteChannel(kir.chanr);

	// Our job is done
	qDebug("Key exchange completed!");
	i->state = Done;
	i->txtimer.stop();

	// Let the ball roll...
	i->fl->start(true);
	i->completed(true);
}

void
KeyInitiator::retransmit(bool fail)
{
	if (fail) {
		//qDebug("Key exchange failed");
		state = Done;
		txtimer.stop();
		return completed(false);
	}

	if (state == I1)
		sendI1();
	else if (state == I2)
		sendDhI2();
	txtimer.restart();
}

