#pragma once

class QByteArray;

// Convergent encryption/decryption
QByteArray convEncrypt(QByteArray &data);
void convDecrypt(QByteArray &data, const QByteArray &key);

QByteArray convEncrypt(const void *in, void *out, int size);
void convDecrypt(const void *in, void *out, int size,
		const QByteArray &key);

// Passkey-based encryption/decryption
QByteArray passEncrypt(const QByteArray &data, const QByteArray &key);
QByteArray passDecrypt(const QByteArray &data, const QByteArray &key);
QByteArray passCheck(const QByteArray &data, const QByteArray &key);
