#ifndef SST_STRM_DGRAM_H
#define SST_STRM_DGRAM_H

#include "strm/abs.h"

namespace SST {


/** @internal Internal pseudo-stream object representing a received datagram.
 * This class makes ephemeral substreams received
 * via SST's optimized datagram-oriented delivery mechanism
 * appear to work like a normal stream
 * that was written and closed immediately.
 */
class DatagramStream : public AbstractStream
{
	Q_OBJECT

private:
	const QByteArray payload;	// Application payload of the datagram
	int pos;			// Current read position in the datagram

	inline int size() const { return payload.size(); }
	inline int remain() const { return size() - pos; }

public:
	DatagramStream(Host *host, const QByteArray &data, int pos);

	virtual bool isLinkUp();

	virtual qint64 bytesAvailable() const
		{ return size() - pos; }
	int readData(char *data, int maxSize);
	int writeData(const char *data, int maxSize,
				quint8 endflags);

	virtual int pendingMessages() const;
	virtual qint64 pendingMessageSize() const;

	virtual int readMessage(char *data, int maxSize);
	virtual QByteArray readMessage(int maxSize);

	virtual bool atEnd() const { return pos >= size(); }


	virtual AbstractStream *openSubstream();
	virtual AbstractStream *acceptSubstream();

	virtual int writeDatagram(const char *data, int size, bool reli);
	virtual int readDatagram(char *data, int maxSize);
	virtual QByteArray readDatagram(int maxSize);


	virtual void shutdown(Stream::ShutdownMode mode);

	virtual void setReceiveBuffer(int size);
	virtual void setChildReceiveBuffer(int size);

#ifndef QT_NO_DEBUG
	/// Dump the state of this stream, for debugging purposes.
	virtual void dump();
#endif
};

} // namespace SST

#endif	// SST_STRM_DGRAM_H
