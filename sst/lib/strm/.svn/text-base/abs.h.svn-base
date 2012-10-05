#ifndef SST_STRM_ABS_H
#define SST_STRM_ABS_H

#include "stream.h"
#include "strm/proto.h"

namespace SST {

class Stream;


/** @internal Abstract base class for internal stream control objects.
 * The separation between the internal stream control object
 * and the application-visible Stream object is primarily needed
 * so that SST can hold onto a stream's state and gracefully shut it down
 * after the application deletes its Stream object representing it.
 * This separation also keeps the internal stream control variables
 * out of the public C++ API header files and thus able to change
 * without breaking binary compatibility,
 * and makes it easy to implement service/protocol negotiation
 * for top-level application streams by extending this class
 * (e.g., see ConnectStream).
 *
 * @see BaseStream, ConnectStream
 */
class AbstractStream : public QObject, public StreamProtocol
{
	friend class Stream;
	Q_OBJECT

protected:	// XXX private w/ accessors?
	typedef Stream::ListenMode ListenMode;


	// Permanent state
	Host *const	h;			// Our per-host state

	// Back-pointer to Stream object, or NULL if Stream has been deleted
	Stream		*strm;

	// EID of peer we're connected to
	QByteArray	peerid;

private:
	int 		pri;		// Current priority level
	ListenMode	lisn;		// Listen for substreams

public:
	/// Create a new AbstractStream.
	AbstractStream(Host *host);

	/// Returns the endpoint identifier (EID) of the local host
	/// as used in connecting the current stream.
	/// Only valid if the stream is connected.
	QByteArray localHostId();

	/// Returns the endpoint identifier (EID) of the remote host
	/// to which this stream is connected.
	QByteArray remoteHostId();

	/// Returns true if the underlying link is currently connected
	/// and usable for data transfer.
	virtual bool isLinkUp() = 0;

	/** Set the stream's transmit priority level.
	 * When the application has multiple streams
	 * with data ready to transmit to the same remote host,
	 * SST uses the respective streams' priority levels
	 * to determine which data to transmit first.
	 * SST gives strict preference to streams with higher priority
	 * over streams with lower priority,
	 * but it divides available transmit bandwidth evenly
	 * among streams with the same priority level.
	 * All streams have a default priority of zero on creation.
	 * @param pri the new priority level for the stream.
	 */
	virtual void setPriority(int pri);

	/// Returns the stream's current priority level.
	inline int priority() { return pri; }


	////////// Byte-oriented Data Transfer //////////

	/** Read up to maxSize bytes of data from the stream.
	 * This method only returns data that has already been received
	 * and is waiting to be read:
	 * it never blocks waiting for new data to arrive on the network.
	 * A single readData() call never crosses a message/record boundary:
	 * if it encounters a record marker in the incoming byte stream,
	 * it returns only the bytes up to that record marker
	 * and leaves any further data
	 * for subsequent readData() calls.
	 *
	 * @param data the buffer into which to read.
	 *		This parameter may be NULL,
	 *		in which case the data read is simply discarded.
	 * @param maxSize the maximum number of bytes to read.
	 * @return the number of bytes read, or -1 if an error occurred.
	 *		Returns zero if there is no error condition
	 *		but no bytes are immediately available for reading.
	 */
	virtual int readData(char *data, int maxSize) = 0;

	/** Write data bytes to a stream.
	 * If not all the supplied data can be transmitted immediately,
	 * it is queued locally until ready to transmit.
	 * @param data the buffer containing the bytes to write.
	 * @param size the number of bytes to write.
	 * @return the number of bytes written (same as the size parameter),
	 * 		or -1 if an error occurred.
	 */
	virtual int writeData(const char *data, int maxSize,
				quint8 endflags) = 0;

	/** Determine the number of bytes currently available to be read
	 * via readData().
	 * Note that calling readData() with a buffer this large
	 * may not read all the available data
	 * if there are message/record markers present in the read stream.
	 */
	virtual qint64 bytesAvailable() const = 0;

	/// Returns true if at least one byte is available for reading.
	inline bool hasBytesAvailable() const { return bytesAvailable() > 0; }

	/** Returns true if all data has been read from the stream
	 * and the remote host has closed its end:
	 * no more data will ever be available for reading on this stream.
	 */
	virtual bool atEnd() const = 0;

	/** Read a complete message all at once.
	 * Reads up to the next message/record marker (or end of stream).
	 * If no message/record marker has arrived yet,
	 * just returns without reading anything.
	 * If the next message to be read is larger than maxSize,
	 * this method simply discards the message data beyond maxSize.
	 * @param data the buffer into which to read the message.
	 * @param maxSize the maximum size of the message to read.
	 * @return the size of the message read, or -1 if an error occurred.
	 *		Returns zero if there is no error condition
	 *		but no complete message is available for reading.
	 */
	virtual int readMessage(char *data, int maxSize) = 0;

	virtual QByteArray readMessage(int maxSize) = 0;

	inline int writeMessage(const char *data, int size)
		{ return writeData(data, size, dataMessageFlag); }

	inline int writeMessage(const QByteArray &msg)
		{ return writeMessage(msg.data(), msg.size()); }

	/// Returns true if at least one complete message
	/// is currently available for reading.
	virtual int pendingMessages() const = 0;
	inline bool hasPendingMessages() const
		{ return pendingMessages() > 0; }

	/// Determine the number of message/record markers
	/// that have been received over the network but not yet read.
	virtual qint64 pendingMessageSize() const = 0;


	////////// Substreams //////////

	/** Initiate a new substream as a child of this stream.
	 * This method completes without synchronizing with the remote host,
	 * and the client application can use the new substream immediately
	 * to send data to the remote host via the new substream.
	 * If the remote host is not yet ready to accept the new substream,
	 * SST queues the new substream and any data written to it locally
	 * until the remote host is ready to accept the new substream.
	 *
	 * By default, the new Stream object's Qt parent object
	 * is the parent Stream object on which openSubstream() was called,
	 * so the substream will automatically be deleted
	 * if the application deletes the parent stream.
	 * The application may re-parent the new Stream object
	 * using QObject::setParent(), however,
	 * without affecting the function of the substream.
	 *
	 * @return a Stream object representing the new substream.
	 */
	virtual AbstractStream *openSubstream() = 0;

	/// Listen for incoming substreams on this stream.
	inline void listen(ListenMode mode) { lisn = mode; }

	/// Returns true if this stream is set to accept incoming substreams.
	inline ListenMode listenMode() { return lisn; }
	inline bool isListening() { return lisn != 0; }

	/** Accept a waiting incoming substream.
	 *
	 * By default, the new Stream object's Qt parent object
	 * is the parent Stream object on which acceptSubstream() was called,
	 * so the substream will automatically be deleted
	 * if the application deletes the parent stream.
	 * The application may re-parent the new Stream object
	 * using QObject::setParent(), however,
	 * in which case the substream may outlive its parent.
	 *
	 * Returns NULL if no incoming substreams are waiting.
	 */
	virtual AbstractStream *acceptSubstream() = 0;

	// Send and receive unordered, unreliable datagrams on this stream.
	virtual int writeDatagram(const char *data, int size, bool reli) = 0;
	virtual int readDatagram(char *data, int maxSize) = 0;
	virtual QByteArray readDatagram(int maxSize) = 0;


	////////// Misc //////////

	// Flow control options
	virtual void setReceiveBuffer(int size) = 0;
	virtual void setChildReceiveBuffer(int size) = 0;

	/** Begin graceful or forceful shutdown of the stream.
	 * If this internal stream control object is "floating" -
	 * i.e., if its 'strm' back-pointer is NULL -
	 * then it should self-destruct once the shutdown is complete.
	 *
	 * @param mode which part of the stream to close:
	 *		either Read, Write, Close (Read|Write), or Reset.
	 */
	virtual void shutdown(Stream::ShutdownMode mode) = 0;


#ifndef QT_NO_DEBUG
	/// Dump the state of this stream, for debugging purposes.
	virtual void dump() = 0;
#endif

protected:
	/// Set an error condition including an error description string.
	inline void setError(const QString &err)
		{ if (strm) strm->setError(err); }
};

} // namespace SST

#endif	// SST_STRM_ABS_H

