
#include <string.h>

#include <QByteArray>

#include <openssl/aes.h>

#include "crypt.h"
#include "sha2.h"

using namespace SST;


static quint8 ctrinit[AES_BLOCK_SIZE] = "VXAcount";

void convDecrypt(const void *in, void *out, int size, const QByteArray &key)
{
	// Set up the AES key schedule
	Q_ASSERT(key.size() == 32);
	AES_KEY akey;
	AES_set_encrypt_key((const unsigned char*)key.data(), 32*8, &akey);

	// Encrypt the block in CTR mode
	unsigned char ivec[AES_BLOCK_SIZE];
	unsigned char ctr[AES_BLOCK_SIZE];
	unsigned num = 0;
	memcpy(ivec, ctrinit, AES_BLOCK_SIZE);
	AES_ctr128_encrypt((const quint8*)in, (quint8*)out, size,
			&akey, ivec, ctr, &num);
}

QByteArray convEncrypt(const void *in, void *out, int size)
{
	// First calculate the encryption key
	QByteArray key = Sha256::hash(in, size);

	// Then CTR-encrypt the block (same as decryption due to XOR)
	convDecrypt(in, out, size, key);

	return key;
}


QByteArray passEncrypt(const QByteArray &data, const QByteArray &key)
{
	Q_ASSERT(false);
	return QByteArray();
}

QByteArray passDecrypt(const QByteArray &data, const QByteArray &key)
{
	Q_ASSERT(false);
	return QByteArray();
}

QByteArray passCheck(const QByteArray &data, const QByteArray &key)
{
	Q_ASSERT(false);
	return QByteArray();
}


