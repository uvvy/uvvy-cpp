
#include <stdint.h>
#include <algorithm>

#include <QtDebug>

#include "opaque.h"
#include "chunk.h"
#include "xdr.h"
#include "util.h"
#include "sha2.h"
#include "adler32.h"
#include "rcsum.h"
#include "crypt.h"

using namespace SST;


//#define MINKEYS		(MINCHUNK/128)	// XX OpaqueKey::xdrsize
//#define MAXKEYS		(MAXCHUNK/128)

#define MINKEYS		4
#define MAXKEYS		8


////////// OpaqueKey //////////

OpaqueKey::OpaqueKey()
:	level(-1)
{
}

OpaqueKey::OpaqueKey(const OpaqueKey &other)
:	level(other.level),
	osize(other.osize),
	isize(other.isize),
	irecs(other.irecs),
	isum(other.isum),
	key(other.key),
	ohash(other.ohash)
{
}

bool OpaqueKey::operator==(const OpaqueKey &other) const
{
	return level == other.level &&
		osize == other.osize &&
		isize == other.isize &&
		irecs == other.irecs &&
		isum == other.isum &&
		key == other.key &&
		ohash == other.ohash;
}

XdrStream &operator>>(XdrStream &xs, OpaqueKey &k)
{
	quint32 reserved;
	xs >> k.level >> k.osize >> k.isize
		>> k.irecs >> k.isum >> reserved;
	k.key.resize(VXA_IHASH_SIZE);
	xs.readRawData(k.key.data(), VXA_IHASH_SIZE);
	k.ohash.resize(VXA_OHASH_SIZE);
	xs.readRawData(k.ohash.data(), VXA_OHASH_SIZE);

	// Do some sanity-checking on the key
	if (k.level < 0 || k.osize <= 0 || k.isize <= 0
			|| k.irecs < 0 || k.irecs > k.isize) {
		k.setNull();
		xs.setStatus(xs.ReadCorruptData);
	}

	return xs;
}

XdrStream &operator<<(XdrStream &xs, const OpaqueKey &k)
{
	Q_ASSERT(!k.isNull());
	const quint32 reserved = 0;
	xs << k.level << k.osize << k.isize
		<< k.irecs << k.isum << reserved;
	Q_ASSERT(k.key.size() == VXA_IHASH_SIZE);
	xs.writeRawData(k.key.data(), VXA_IHASH_SIZE);
	Q_ASSERT(k.ohash.size() == VXA_OHASH_SIZE);
	xs.writeRawData(k.ohash.data(), VXA_OHASH_SIZE);
	return xs;
}


////////// OpaqueInfo //////////

OpaqueInfo::OpaqueInfo(const OpaqueInfo &other)
:	t(other.t),
	nrecs(other.nrecs),
	dat(other.dat),
	kl(other.kl)
{
}

OpaqueInfo OpaqueInfo::direct(const QByteArray &inlineData, int nrecords)
{
	Q_ASSERT(nrecords > 0 && nrecords <= inlineData.size());

	OpaqueInfo i;
	i.t = nrecords == 1 ? Inline1 : InlineRecs;
	i.nrecs = nrecords;
	i.dat = inlineData;
	return i;
}

OpaqueInfo OpaqueInfo::indirect(const QList<OpaqueKey> &indirectKeys)
{
	OpaqueInfo i;
	i.t = Indirect;
	i.kl = indirectKeys;
	return i;
}

qint64 OpaqueInfo::internalSize() const
{
	switch (t) {
	case Inline1:
	case InlineRecs:
		return dat.size();
	case Indirect: {
		qint64 totsize = 0;
		for (int i = 0; i < kl.size(); i++)
			totsize += kl[i].isize;
		return totsize; }
	default:
		return 0;
	}
}

qint64 OpaqueInfo::internalRecords() const
{
	switch (t) {
	case Inline1:
		Q_ASSERT(nrecs == 1);
	case InlineRecs:
		return nrecs;
	case Indirect: {
		qint64 totrecs = 0;
		for (int i = 0; i < kl.size(); i++)
			totrecs += kl[i].irecs;
		return totrecs; }
	default:
		return 0;
	}
}

quint32 OpaqueInfo::internalSum() const
{
	switch (t) {
	case Inline1:
	case InlineRecs:
		return adler32(dat.data(), dat.size());
	case Indirect: {
		quint32 sum = ADLER32_INIT;
		for (int i = 0; i < kl.size(); i++)
			sum = adler32cat(sum, kl[i].isum, kl[i].isize);
		return sum; }
	default:
		return 0;
	}
}

int OpaqueInfo::directSize(int dataSize, int nrecords)
{
	// 4-byte type, 4-byte record count, 4-byte length, padded data
	return 4 + 4 + (nrecords > 1 ? 4 : 0) + ((dataSize + 3) & ~3);
}

int OpaqueInfo::indirectSize(int nkeys)
{
	// 4-byte type, 4-byte key count, array of encoded keys
	return 4 + 4 + OpaqueKey::xdrsize * nkeys;
}

QByteArray OpaqueInfo::encode() const
{
	QByteArray buf;
	XdrStream ws(&buf, QIODevice::WriteOnly);
	ws << *this;
	Q_ASSERT(ws.status() == ws.Ok);
	return buf;
}

OpaqueInfo OpaqueInfo::decode(const QByteArray &metaData)
{
	XdrStream rs(metaData);
	OpaqueInfo oi;
	rs >> oi;
	return oi;
}

void OpaqueInfo::encode(XdrStream &ws) const
{
	switch (t) {
	case Inline1:
		Q_ASSERT(nrecs == 1);
		ws << (quint32)Inline1 << dat;
		break;
	case OpaqueInfo::InlineRecs:
		ws << (quint32)InlineRecs << (quint32)nrecs << dat;
		break;
	case Indirect:
		ws << (quint32)Indirect << (qint32)kl.size();
		for (int i = 0; i < kl.size(); i++)
			ws << kl[i];
		break;
	default:
		Q_ASSERT(0);
	}
}

void OpaqueInfo::decode(XdrStream &rs)
{
	// Produce a null OpaqueInfo by default on error
	t = Null;
	nrecs = 1;

	// Decode the type discriminant
	qint32 type;
	rs >> type;
	switch ((Type)type) {

	case InlineRecs:
		rs >> nrecs;
		// fall through...

	case Inline1: {

		// Direct inline data
		rs >> dat;
		if (rs.status() != rs.Ok)
			return;
		if (nrecs <= 0 || nrecs > dat.size()) {
			rs.setStatus(rs.ReadCorruptData);
			return;
		}

		// Initialize to the correct type
		this->t = (Type)type;
		break; }

	case OpaqueInfo::Indirect: {

		// Indirect metadata - get the key count.
		qint32 nkeys;
		rs >> nkeys;
		if (rs.status() != rs.Ok)
			return;
		if (nkeys <= 0) {
			rs.setStatus(rs.ReadCorruptData);
			return;
		}

		// Deserialize the individual keys.
		kl.clear();
		for (int i = 0; i < nkeys; i++) {
			OpaqueKey k;
			rs >> k;
			if (rs.status() != rs.Ok)
				return;
			kl.append(k);
		}

		// Initialize to the correct type
		this->t = Indirect;
		break; }

	default:
		rs.setStatus(rs.ReadCorruptData);
		return;
	}
}


////////// OpaqueCoder //////////

OpaqueCoder::OpaqueCoder(QObject *parent)
:	QIODevice(parent),
	totsize(0), totrecs(0),
	adleracc(ADLER32_INIT), adlerchk(ADLER32_INIT),
	avail(0)
{
	setOpenMode(WriteOnly);

	// The first record starts at the beginning of the opaque stream.
	marks.append(0);
}

OpaqueCoder::~OpaqueCoder()
{
}

bool OpaqueCoder::open(OpenMode)
{
	Q_ASSERT(0);
	return false;
}

bool OpaqueCoder::isSequential() const
{
	return true;
}

qint64 OpaqueCoder::pos() const
{
	return totsize + avail;
}

qint64 OpaqueCoder::readData (char *, qint64)
{
	Q_ASSERT(0);
	return -1;
}

qint64 OpaqueCoder::writeData(const char *data, qint64 datasize)
{
	// Make sure our buffer is big enough
	int reqbufsize = std::min(avail + datasize, (qint64)maxChunkSize);
	if (buf.size() < reqbufsize)
		buf.resize(reqbufsize);

	// Repeatedly fill up our chunk buffer and process chunks
	// as long as we have enough input data.
	int remain = datasize;
	while (avail + remain >= maxChunkSize) {
		Q_ASSERT(avail < maxChunkSize);

		// Fill up the chunk buffer
		int amount = maxChunkSize - avail;
		memcpy(buf.data() + avail, data, amount);
		data += amount;
		remain -= amount;
		avail = maxChunkSize;

		// Extract and write a chunk from the current buffer
		if (!doChunk())
			return -1;
	}

	// Leave any remaining input data in our chunk buffer
	memcpy(buf.data() + avail, data, remain);
	avail += remain;

	return datasize;
}

qint64 OpaqueCoder::readFrom(QIODevice *in, qint64 maxsize)
{
	if (maxsize < 0)
		maxsize = (qint64)1 << 62;
	if (buf.size() < maxChunkSize)
		buf.resize(maxChunkSize);

	// Push data into our buffer as long as it's available
	qint64 actsize = 0;
	while (maxsize > 0 && !in->atEnd()) {
		Q_ASSERT(avail < maxChunkSize);

		// See how much we can fit in the buffer in this round
		int amount = maxChunkSize - avail;
		if (amount > maxsize)
			amount = maxsize;

		// Read it in
		int actual = in->read(buf.data() + avail, amount);
		if (actual < 0) {
			err = in->errorString();
			return -1;
		}
		avail += actual;
		actsize += actual;
		maxsize -= actual;

		// If we've filled our chunk buffer, emit a chunk.
		if (avail >= maxChunkSize) {
			if (!doChunk())
				return -1;
		}
	}
	return actsize;
}

void OpaqueCoder::markRecord()
{
	// If this chunk is a continuation of a previous record,
	// split it here and start the next record on a chunk boundary.
	int nmarks = marks.size();
	if (nmarks == 0 && avail > 0) {
		doChunk();
		Q_ASSERT(avail == 0);
	}
	Q_ASSERT(nmarks == 0 || (marks[0] == 0 && marks[nmarks-1] < avail));

	// Record all chunk markers within this chunk.
	marks.append(avail);
}

bool OpaqueCoder::doChunk()
{
	Q_ASSERT(avail > 0 && avail <= maxChunkSize);

	// Determine the point at which to split the chunk.
	int splitsize, splitrecs;
	int newrecs = marks.size();
	if (avail < maxChunkSize) {

		// Chunk is already smaller than the maximum,
		// e.g., because we're at the end of the opaque stream
		// or at the last chunk in a multi-chunk record.
		// No need to split further.
		splitsize = avail;

		// Consume all outstanding record markers,
		// except for any last one pointing to the end of the chunk.
		if (newrecs > 0 && marks[newrecs-1] == avail)
			splitrecs = newrecs-1;
		else
			splitrecs = newrecs;

	} else if (newrecs <= 1) {

		// This chunk is the beginning of a long record (newrecs == 1)
		// or the continuation of a long record (newrecs == 0).
		// Split the record convergently at byte granularity.
		Q_ASSERT(newrecs == 0 || marks[0] == 0);
		splitsize = RollingChecksum::scanChunk(buf.data(),
						minChunkSize, maxChunkSize);
		splitrecs = newrecs;

	} else if (marks[newrecs-1] < minChunkSize) {

		// We have two or more records in the buffer,
		// so we want to split on a record boundary -
		// but the last marker comes too early to split by content.
		// Just split before the large, still-incomplete last record.
		splitsize = marks[newrecs-1];
		splitrecs = newrecs-1;

	} else {

		// Use a rolling checksum to find the best split point,
		// from among the record boundaries we have available,
		// which will result in a chunk of at least minChunkSize.

		// Find the first eligible split point.
		Q_ASSERT(marks[0] == 0);
		int i = 1;
		while (marks[i] < minChunkSize) {
			Q_ASSERT(marks[i] > marks[i-1]);
			i++;
		}

		// Initialize the rolling checksum at that split point.
		RollingChecksum rc(buf.data(),
				marks[i] - minChunkSize, marks[i]);
		Q_ASSERT(rc.slabSize() == minChunkSize);
		splitsize = marks[i];
		splitrecs = i;
		quint32 splitsum = rc.sum();

		// Scan for other eligible split points,
		// and find the one with the lowest rolling sum.
		while (++i < newrecs) {
			Q_ASSERT(marks[i] > marks[i-1]);
			rc.advanceEnd(marks[i]);
			if (rc.sum() <= splitsum) {
				splitsize = marks[i];
				splitrecs = i;
				splitsum = rc.sum();
			}
		}
	}
	Q_ASSERT(splitsize > 0 && splitsize <= avail);
	Q_ASSERT(splitrecs >= 0 && splitrecs <= newrecs);

	// Convergently encrypt the chunk
	QByteArray cyph;
	OpaqueKey k = encDataChunk(buf.data(), splitsize, splitrecs, cyph);

	// Push the data chunk's key into our key list.
	if (!addKey(k))
		return false;

	// Pass the data chunk to our "consumer", whoever it is
	if (!dataChunk(cyph, k, totsize, splitsize))
		return false;

	// Progressively compute a checksum of the entire input.
	adleracc = adler32cat(adleracc, k.isum, splitsize);

	// Sanity check for our adler32 composition mechanism...
	// XXX bad for efficiency.
	adlerchk = adler32(buf.data(), splitsize, adlerchk);
	Q_ASSERT(adlerchk == adleracc);

	// Proceed to next chunk
	memmove(buf.data(), buf.data() + splitsize, avail - splitsize);
	avail -= splitsize;
	totsize += splitsize;

	// Consume the appropriate number of record markers
	QList<int> oldmarks = marks;
	marks.clear();
	for (int i = splitrecs; i < newrecs; i++)
		marks.append(oldmarks[i] - splitsize);
	totrecs += splitrecs;

	return true;
}

bool OpaqueCoder::addKey(const OpaqueKey &key)
{
	// Append the key to the appropriate level.
	if (key.level >= keys.size()) {
		// First key at this level.
		Q_ASSERT(key.level == keys.size());
		keys.append(QList<OpaqueKey>());
	}
	QList<OpaqueKey> &kl = keys[key.level];
	kl.append(key);

	// We're done unless this level collects too many keys.
	if (kl.size() <= MAXKEYS)
		return true;

	// This level is too full - split the first part into a key chunk.
	// Since each key's ohash should be a strong unique identifier,
	// just use the first few bits of it to choose a split-point.
	int selend = MINKEYS;
	quint64 selcheck = hashCheck(kl[MINKEYS-1].ohash);
	for (int i = MINKEYS; i < MAXKEYS; i++) {
		quint64 check = hashCheck(kl[i].ohash);
		if (check <= selcheck) {
			selend = i+1;
			selcheck = check;
		}
	}

	// Build a new key-chunk based on the first part of the key list.
	QList<OpaqueKey> nkl = kl.mid(0, selend);
	QByteArray nkcyph;
	OpaqueKey nk = encKeyChunk(nkl, nkcyph);

	// Push the new key-chunk's key up to the next level
	if (!addKey(nk))
		return false;

	// Update our still-to-be-written key list at this level
	kl = kl.mid(selend);

	// Pass the new key chunk to our consumer
	if (!keyChunk(nkcyph, nk, nkl))
		return false;

	return true;
}

OpaqueInfo OpaqueCoder::finish(int metaDataLimit)
{
	Q_ASSERT(metaDataLimit >= minMetaDataLimit);
	Q_ASSERT(minMetaDataLimit == OpaqueInfo::indirectSize(1));

	// Find the number of marks in the last chunk,
	// not including one that might occur at the end of a non-empty file.
	int nrecs = marks.size();
	if (nrecs > 1 && marks[nrecs-1] == avail)
		nrecs--;

	// If the whole file fits into our meta-data limit, just inline it.
	if (keys.isEmpty() && (OpaqueInfo::directSize(avail, nrecs)
					<= metaDataLimit)) {
		qDebug("Inlining small %d-byte file", avail);
		buf.resize(avail);
		return OpaqueInfo::direct(buf, nrecs);
	}

	// Otherwise, flush the current data chunk, if any.
	if (avail > 0)
		doChunk();
	Q_ASSERT(avail == 0);

	// We should have at least one level of keys now.
	int nlevs = keys.size();
	Q_ASSERT(nlevs > 0);

	// Merge all levels upwards, until we end up at the top-level
	// with few enough keys to fit into the metaDataLimit.
	for (int lev = 0; ; lev++) {
		Q_ASSERT(lev < keys.size());
		QList<OpaqueKey> &kl = keys[lev];
		int nkeys = kl.size();
		Q_ASSERT(nkeys > 0 && nkeys <= MAXKEYS);

		// We're done when we end up with one key on the last level.
		if (OpaqueInfo::indirectSize(nkeys) <= metaDataLimit
				&& lev == keys.size()-1) {

			OpaqueInfo i = OpaqueInfo::indirect(kl);
			Q_ASSERT(i.internalSize() == totsize);
			Q_ASSERT(i.internalRecords() == totrecs);
			Q_ASSERT(i.internalSum() == adleracc);

			keys.clear();
			buf.clear();
			setOpenMode(NotOpen);

			return i;
		}

		// Merge all the keys in this level into one key chunk,
		// adding the resulting key to the next level.
		QByteArray nkcyph;
		OpaqueKey nk = encKeyChunk(kl, nkcyph);
		if (!addKey(nk))
			return OpaqueInfo();
		if (!keyChunk(nkcyph, nk, kl))
			return OpaqueInfo();
		kl.clear();
	}
}

OpaqueInfo OpaqueCoder::encode(QIODevice *in, int metaDataLimit)
{
	if (readFrom(in) < 0)
		return OpaqueInfo();

	return finish(metaDataLimit);
}

void OpaqueCoder::encChunk(const void *data, int size, QByteArray &cyph,
		QByteArray &key, QByteArray &ohash)
{
	cyph.resize(size);
	key = convEncrypt(data, cyph.data(), size);
	ohash = Sha512::hash(cyph);
}

void OpaqueCoder::encChunk(const QByteArray &clear, QByteArray &cyph,
		QByteArray &key, QByteArray &ohash)
{
	encChunk(clear.data(), clear.size(), cyph, key, ohash);
}

OpaqueKey OpaqueCoder::encDataChunk(const void *data, int size, qint64 recs,
			QByteArray &cyph)
{
	// Prepare an OpaqueKey to describe the data chunk.
	OpaqueKey k;
	k.level = 0;
	k.osize = size;
	k.isize = size;
	k.irecs = recs;

	// Compute a standard Adler32 checksum over the cleartext,
	// as a post-decryption (and in future versions post-decompression)
	// sanity check on the final recovered content.
	// Adler32 is handy here because it composes nicely.
	k.isum = adler32(data, size);

	// Convergently encrypt the data chunk and produce its key and hash.
	encChunk(data, size, cyph, k.key, k.ohash);

	return k;
}

OpaqueKey OpaqueCoder::encKeyChunk(const QList<OpaqueKey> &keys,
			QByteArray &cyph)
{
	int level = keys[0].level + 1;
	int nkeys = keys.size();
	Q_ASSERT(nkeys > 0);

	// Prepare an OpaqueKey to describe the key chunk.
	// All the keys in the provided list must be of the same level.
	OpaqueKey k;
	k.level = keys[0].level + 1;

	// Encode the list of keys into a cleartext buffer,
	// and compute the internal size and Adler32 checksums.
	k.isize = 0;
	k.irecs = 0;
	k.isum = ADLER32_INIT;
	QByteArray buf;
	XdrStream ws(&buf, QIODevice::WriteOnly);
	for (int i = 0; i < nkeys; i++) {
		const OpaqueKey &subk = keys[i];
		Q_ASSERT(level == subk.level + 1);
		k.isize += subk.isize;
		k.irecs += subk.irecs;
		k.isum = adler32cat(k.isum, subk.isum, subk.isize);
		ws << subk;
	}
	k.osize = buf.size();
	Q_ASSERT(k.osize == nkeys * OpaqueKey::xdrsize);

	// Convergently encrypt the chunk and produce its key and hash.
	encChunk(buf, cyph, k.key, k.ohash);

	return k;
}


////////// AbstractOpaqueReader //////////

void AbstractOpaqueReader::readBytes(const OpaqueInfo &info,
					qint64 start, qint64 size)
{
	qint64 end = start + size;
	Q_ASSERT(start >= 0);
	Q_ASSERT(size > 0);
	Q_ASSERT(end > start);
	Q_ASSERT(end <= info.internalSize());

	readRegion(info, start, end, false);
}

void AbstractOpaqueReader::readRecords(const OpaqueInfo &info,
					qint64 recstart, qint64 nrecs)
{
	qint64 recend = recstart + nrecs;
	Q_ASSERT(recstart >= 0);
	Q_ASSERT(nrecs > 0);
	Q_ASSERT(recend > recstart);
	Q_ASSERT(recend <= info.internalRecords());

	readRegion(info, recstart, recend, true);
}

void AbstractOpaqueReader::readRegion(const OpaqueInfo &info,
				qint64 start, qint64 end, bool byrec)
{
	if (info.isInline()) {
		gotData2(QByteArray(), 0, 0, info.inlineData(), info.internalRecords());
		return readDone();
	}

	Q_ASSERT(info.isIndirect());

	Region r;
	r.info = info;
	r.byteofs = r.recofs = 0;
	r.start = start;
	r.end = end;
	r.byrec = byrec;
	return addKeys(r, info.indirectKeys());

	// If all needed blocks are cached, we might be done immediately.
	if (regions.isEmpty())
		readDone();
}

void AbstractOpaqueReader::addKeys(const Region &r,
				const QList<OpaqueKey> &keys)
{
	qint64 byteofs = r.byteofs, recofs = r.recofs;
	int nkeys = keys.size();
	QList<QByteArray> hashes;
	for (int i = 0; i < nkeys; i++) {

		// Stop if we're already past the part we're interested in.
		if (r.byrec ? (recofs >= r.end) : (byteofs >= r.end))
			break;

		const OpaqueKey &k = keys[i];
		if (k.level < 0 || k.osize <= 0 ||
				k.isize <= 0 || k.irecs < 0) {
			qDebug() << "OpaqueReader: invalid key!";
			continue;
		}

		// See if this subregion is in range
		if (r.byrec ? (recofs + k.irecs > r.start)
			    : (byteofs + k.isize > r.start)) {

			qDebug() << "Add level" << k.level << "region"
				<< byteofs << "to" << (byteofs + k.isize);

			// We need this subregion.
			Region subr;
			subr.info = r.info;
			subr.byteofs = byteofs;
			subr.recofs = recofs;
			subr.key = k;
			subr.start = r.start;
			subr.end = r.end;
			subr.byrec = r.byrec;
			regions.insert(k.ohash, subr);
			hashes.append(k.ohash);
		}

		// Move to next subregion
		byteofs += k.isize;
		recofs += k.irecs;
	}

	// Finally, request all the chunks we need to fill the subregions.
	// It's important that we do this at the very end instead of above:
	// otherwise the readChunk() calls may succeed synchronously
	// and confuse us into thinking we're done prematurely.
	foreach (const QByteArray &hash, hashes)
		readChunk(hash);
}

void AbstractOpaqueReader::gotData(const QByteArray &ohash,
				const QByteArray &cyph)
{
	int size = cyph.size();

	// Only decrypt the chunk once (as long as the key info matches).
	OpaqueKey k;
	QByteArray buf;
	QList<OpaqueKey> kl;

	// Find all the read-regions that were waiting for this chunk.
	foreach (const Region &r, regions.values(ohash)) {

		// Clear the cached information if the key information changes:
		// shouldn't happen except if someone's metadata is bad.
		if (r.key != k) {
			k = r.key;
			buf.clear();
			kl.clear();
		}

		// Make sure the chunk size matches what we were expecting -
		// if not, the hash algorithm is broken, or the metadata is bad.
		if (k.osize != size) {
			qDebug("OpaqueKey outer chunk size mismatch");
			bad:
			noData2(ohash, r.byteofs, r.recofs, k.isize, k.irecs);
			continue;
		}

		// Decrypt the chunk data.
		if (buf.isEmpty()) {
			buf.resize(size);
			convDecrypt(cyph.constData(), buf.data(), size, k.key);
		}

		if (k.level == 0) {
			// It's an actual data chunk.

			// Check the Adler32 checksum out of paranoia.
			if (adler32(buf.data(), size) != k.isum) {
				qDebug("Bad Adler32 checksum on data chunk");
				goto bad;
			}

			// Pass the data up to the client, and we're done.
			gotData2(ohash, r.byteofs, r.recofs, buf, k.irecs);
			continue;
		}

		// It's a key chunk.  Decode the subkeys it contains.
		int nkeys = size / OpaqueKey::xdrsize;
		if (kl.isEmpty()) {
			if (nkeys == 0 || size % OpaqueKey::xdrsize) {
				qDebug("Bad key chunk size");
				goto bad;
			}
			XdrStream rs(buf);
			qint64 totsize = 0;
			for (int i = 0; i < nkeys; i++) {
				OpaqueKey subk;
				rs >> subk;
				if (rs.status() != rs.Ok ||
						subk.isNull()) {
					qDebug("Bad key in key chunk");
					goto bad;
				}
				if (totsize + subk.isize <= totsize) {
					qDebug("Bad isize in key chunk");
					goto bad;
				}
				totsize += subk.isize;
				kl.append(subk);
			}
			Q_ASSERT(rs.atEnd());
			if (totsize != k.isize) {
				qDebug("Key chunk internal sizes don't add up");
				goto bad;
			}
		}
		Q_ASSERT(kl.size() == nkeys);

		// Add and start reading a new subregion for each subkey.
		addKeys(r, kl);

		// Also make the metadata chunk available to the subclass
		gotMetaData(ohash, cyph, r.byteofs, r.recofs, k.isize, k.irecs);
	}
	regions.remove(ohash);

	// Notify the client if we run out of stuff to read.
	if (regions.isEmpty())
		readDone();
}

void AbstractOpaqueReader::noData(const QByteArray &ohash)
{
	// Notify all the read-regions that were waiting for this chunk.
	QList<Region> rl = regions.values(ohash);
	regions.remove(ohash);
	foreach (const Region &r, rl) {
		noData2(ohash, r.byteofs, r.recofs, r.key.isize, r.key.irecs);
	}

	// Notify the client if we run out of stuff to read.
	if (regions.isEmpty())
		readDone();
}

void AbstractOpaqueReader::readDone()
{
}

void AbstractOpaqueReader::gotMetaData(const QByteArray &, const QByteArray &,
			qint64, qint64, qint64, int)
{
	// Default implementation does nothing
}

