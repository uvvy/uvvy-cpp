
#include <QtDebug>

#include "strm/dgram.h"

using namespace SST;


////////// DatagramStream //////////

DatagramStream::DatagramStream(Host *host, const QByteArray &payload, int pos)
:	AbstractStream(host),
	payload(payload),
	pos(pos)
{
}

bool DatagramStream::isLinkUp()
{
	return false;	// already closed
}

int DatagramStream::readData(char *data, int maxSize)
{
	int act = qMin(remain(), maxSize);
	memcpy(data, payload.constData() + pos, act);
	pos += act;
	return act;
}

int DatagramStream::writeData(const char *, int, quint8)
{
	setError("Can't write to ephemeral datagram-streams");
	return -1;
}

int DatagramStream::pendingMessages() const
{
	return (size() > pos) ? 1 : 0;
}

qint64 DatagramStream::pendingMessageSize() const
{
	return remain();
}

int DatagramStream::readMessage(char *data, int maxSize)
{
	// A datagram can contain only one message by definition.
	// So read whatever of it the caller wants and discard the rest.
	int act = DatagramStream::readData(data, maxSize);
	pos = size();
	return act;
}

QByteArray DatagramStream::readMessage(int maxSize)
{
	if (pos == 0 && maxSize >= size()) {
		// The quick and easy case...
		pos = size();
		return payload;
	}

	QByteArray buf;
	int act = qMin(remain(), maxSize);
	int oldpos = pos;
	pos += act;
	return payload.mid(oldpos, act);
}

AbstractStream *DatagramStream::openSubstream()
{
	setError("Ephemeral datagram-streams cannot have substreams");
	return NULL;
}

AbstractStream *DatagramStream::acceptSubstream()
{
	setError("Ephemeral datagram-streams cannot have substreams");
	return NULL;
}

int DatagramStream::writeDatagram(const char *, int, bool)
{
	setError("Ephemeral datagram-streams cannot have sub-datagrams");
	return -1;
}

int DatagramStream::readDatagram(char *, int)
{
	setError("Ephemeral datagram-streams cannot have sub-datagrams");
	return -1;
}

QByteArray DatagramStream::readDatagram(int)
{
	setError("Ephemeral datagram-streams cannot have sub-datagrams");
	return QByteArray();
}

void DatagramStream::shutdown(Stream::ShutdownMode mode)
{
	if (mode & (Stream::Reset | Stream::Read))
		pos = size();

	if (strm == NULL) {
		//qDebug() << this << "self-destructing";
		deleteLater();
	}
}

void DatagramStream::setReceiveBuffer(int)
{
	// do nothing
}

void DatagramStream::setChildReceiveBuffer(int)
{
	// do nothing
}

#ifndef QT_NO_DEBUG
void DatagramStream::dump()
{
	qDebug() << this << "pos" << pos << "size" << size();
}
#endif

