/** @file stream.h Public API definitions for SST streams.
 * The stream is the primary high-level communication abstraction
 * that the SST provides to the application.
 */
#ifndef SST_C_STREAM_H
#define SST_C_STREAM_H

#include <sst/decls.h>



enum sst_shutdownmode {
	sst_shutdown_read	= 1,	// Read (incoming data) direction
	sst_shutdown_write	= 2,	// Write (outgoing data) direction
	sst_shutdown_close	= 3,	// Both directions (Read|Write)
	sst_shutdown_reset	= 4,	// Forceful reset
};



/** This class represents an SST stream.
 * This is the primary high-level class that client applications use
 * to communicate over the network via SST.
 * The class provides standard stream-oriented read/write functionality
 * via the methods in its QIODevice base class,
 * and adds SST-specific methods for controlling SST streams and substreams.
 *
 * To initiate an outgoing "top-level" SST stream to a remote host,
 * the client application creates a Stream instance
 * and then calls connectTo().
 * To initiate a sub-stream from an existing stream,
 * the application calls openSubstream() on the parent stream.
 *
 * To accept incoming top-level streams from other hosts
 * the application creates a StreamServer instance,
 * and that class creates Stream instances for incoming connections.
 * To accept new incoming substreams on existing streams,
 * the application calls listen() on the parent stream,
 * and upon arrival of a newSubstream() signal
 * the application calls acceptSubstream()
 * to obtain a Stream object for the new incoming substream.
 *
 * SST uses service and protocol names
 * in place of the port numbers used by TCP and UDP
 * to differentiate and select among different application protocols.
 * A service name represents an abstract service being provided:
 * e.g., "Web", "File", "E-mail", etc.
 * A protocol name represents a concrete application protocol
 * to be used for communication with an abstract service:
 * e.g., "HTTP 1.0" or "HTTP 1.1" for communication with a "Web" service;
 * "FTP", "NFS v4", or "CIFS" for communication with a "File" service;
 * "SMTP", "POP3", or "IMAP4" for communication with an "E-mail" service.
 * Service names are intended to be suitable for non-technical users to see,
 * in a service manager or firewall configuration utility for example,
 * while protocol names are primarily intended for application developers.
 * A server can support multiple distinct protocols on one logical service,
 * for backward compatibility or functional modularity reasons for example,
 * by registering to listen on multiple (service, protocol) name pairs.
 *
 * @see StreamServer
 */
typedef struct sst_stream sst_stream;


/** Create a new Stream instance.
 * The Stream must be connected to a remote host via connectTo()
 * before it can be used for communication.
 * @param host the Host instance containing hostwide SST state.
 * @param parent the Qt parent for the new object.
 *		The Stream will be automatically destroyed
 *		when its parent is destroyed.
 */
sst_stream *sst_stream_create(sst_host *host);

/** Destroy a stream. */
void sst_stream_free(sst_stream *strm);


////////// Connection //////////

/** Connect to a given service and protocol on a remote host.
 * The stream logically goes into the "connected" state immediately
 * (i.e., isConnected() returns true),
 * and the application may start writing to the stream immediately,
 * but actual network connectivity may not be established
 * until later or not at all.
 * Either during or some time after the connectTo() call,
 * SST emits the linkUp() signal to indicate active connectivity,
 * or linkDown() to indicate connectivity could not be established.
 * A linkDown() signal is not necessarily fatal, however:
 * unless the application disconnects or deletes the Stream object,
 * SST will continue attempting to establish connectivity
 * and emit linkUp() if and when it eventually succeeds.
 *
 * If the stream is already connected when connectTo() is called,
 * SST immediately re-binds the Stream object to the new target
 * but closes the old stream gracefully in the background.
 * Similarly, SST closes the stream gracefully in the background
 * if the application just deletes a connected Stream object.
 * To close a stream forcefully without retaining internal state,
 * the application may explicitly call shutdown(Reset)
 * before re-connecting or deleting the Stream object.
 * 
 * @param dstid the endpoint identifier (EID)
 *		of the desired remote host to connect to.
 *		The destination may be either a cryptographic EID
 * 		or a non-cryptographic legacy address
 *		as defined by the Ident class.
 *		The dstid may also be empty,
 *		indicating that the destination's identity is unknown;
 *		in this case the caller must provide a location hint
 *		via the dstep argument.
 * @param service the service name to connect to on the remote host.
 * @param protocol the application protocol name to connect to.
 * @param dstep	an optional location hint
 *		for SST to use in attempting to contact the host.
 *		If the dstid parameter is a cryptographic EID,
 *		which is inherently location-independent,
 *		SST may need a location hint to find the remote host
 *		if this host and the remote host are not currently
 *		registered at a common registration server,
 *		for example.
 *		This parameter is not needed
 *		if the dstid is a non-cryptographic legacy address.
 * @return true if successful, false if an error occurred.
 * @see Ident
 */
int sst_stream_connect(sst_stream *strm, const sst_id *id, int idlen,
			const char *service, const char *protocol);

/* XXX		const QList<Endpoint> &dsteps = QList<Endpoint>()); */

/** Disconnect the stream from its current peer.
 * This method immediately returns the stream to the unconnected state:
 * isConnected() subsequently returns false.
 * If the stream has not already been shutdown, however,
 * SST gracefully closes the stream in the background
 * as if with shutdown(Close).
 * @see shutdown()
 */
int sst_stream_disconnect(sst_stream *strm);

/** Returns true if the Stream is logically connected
 * and usable for data read/write operations.
 * The return value from this method changes only as a result of
 * the application's calls to connectTo() and disconnect().
 * Logical connectivity does not imply that the network link is live:
 * the underlying link may go up or down repeatedly
 * during the logical lifetime of the stream.
 */
int sst_stream_isconnected(sst_stream *strm);


/** Give the stream layer a location hint for a specific EID,
 * which may or may not be the EID of the host
 * to which this stream is currently connected (if any).
 * The stream layer will use this hint in any current or subsequent
 * attempts to connect to the specified EID.
 */
/** Provide a new or additional address hint.
 * May be called at any time, e.g., if the target host has migrated,
 * to give SST a hint at where it might find the target
 * in order to re-establish connectivity. */
int sst_stream_addrhint(sst_stream *strm, const sst_id *id, int idlen;
			const struct sockaddr *sa, int salen);


////////// Reading Data //////////

/** Determine the number of bytes currently available to be read
 * via readData().
 * Note that calling readData() with a buffer this large
 * may not read all the available data
 * if there are message/record markers present in the read stream.
 */
ssize_t sst_stream_avail(sst_stream *strm);

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
ssize_t sst_stream_read(sst_stream *strm, void *buf, size_t maxsize);


/** Returns true if all data has been read from the stream
 * and the remote host has closed its end:
 * no more data will ever be available for reading on this stream.
 */
int sst_stream_atend(sst_stream *strm);


////////// Writing Data //////////

/** Write data bytes to a stream.
 * If not all the supplied data can be transmitted immediately,
 * it is queued locally until ready to transmit.
 * @param data the buffer containing the bytes to write.
 * @param size the number of bytes to write.
 * @return the number of bytes written (same as the size parameter),
 * 		or -1 if an error occurred.
 */
int sst_stream_write(sst_stream *strm, const char *buf, size_t size);

// Send and receive unordered datagrams on this stream.
// Reliability is optional.  (XX use enum instead of bool to choose.)
int sst_stream_recv(sst_stream *strm, void *buf, int size, int flags);
int sst_stream_send(sst_stream *strm, const void *buf, int size, int flags);


// Check for pending data and/or datagrams to read, bytes queued to write, ...
int sst_stream_status(sst_stream *strm);


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
sst_stream *sst_stream_spawn(sst_stream *strm);

/// Listen for incoming substreams on this stream.
int sst_stream_listen(sst_stream *strm);

// Set or clear the default-listen flag that determines
// whether new substreams of this stream listen by default.
int sst_stream_setflags(sst_stream *strm, int flag);
int sst_stream_getflags(sst_stream *strm);

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
sst_stream *sst_stream_accept(sst_stream *strm);


////////// Stream Control //////////

struct sst_streaminfo
{
	const sst_id *localid;
	const sst_id *remoteid;
	int localidlen;
	int remoteidlen;

	// current link status
	sst_linkstatus linkstat;

	// current transfer priority for this stream
	int pri;

	// maximum receive window size
	int window;
};

/// Returns the endpoint identifier (EID) of the local host
/// as used in connecting the current stream.
/// Only valid if the stream is connected.
sst_streaminfo *sst_stream_info(sst_stream *strm);

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
int sst_stream_setpri(sst_stream *strm, int pri);

/** Control the maximum receive window size for this stream.
 */
int sst_stream_setwindow(sst_stream *strm, int window);

/** Begin graceful or forceful shutdown of the stream.
 * To close the stream gracefully in either or both directions,
 * specify Read, Write, or Read|Write for the @a mode argument.
 * Closing the stream in the Write direction 
 * writes the end-of-stream marker to our end of the stream,
 * indicating to our peer that no more data will arrive from us.
 * Closing the stream in the Read direction discards any data
 * waiting to be read or subsequently received from the peer.
 * Specify a mode of @a Reset to shutdown the stream immediately;
 * written data that is still queued or in transit may be lost.
 *
 * @param dir which part of the stream to close:
 *		either Read, Write, Close (same as Read|Write),
 *		or Reset.
 */
int sst_stream_shutdown(sst_stream *strm, sst_shutdownmode mode);

/// Gracefully close the stream for both reading and writing.
/// Still-buffered written data continues to be sent,
/// but any further data received from the other side is dropped.
static inline int sst_stream_close(sst_stream *strm)
{
	sst_stream_shutdown(sst_shutdown_close);
}


////////// Signals //////////
signals:

/** Emitted when a message/record marker arrives
 * in the incoming byte stream ready to be read.
 * This signal indicates that a complete record may be read at once.
 * If the client wishes to delay the reading of any data
 * in the message or record until this signal arrives,
 * to avoid the potential for deadlock the client must ensure that
 * the stream's maximum receive window is large enough to accommodate
 * any message or record that might arrive -
 * or else monitor the receiveBlocked() signal
 * and dynamically expand the receive window as necessary.
 */
void readyReadMessage();

#if 0	// XXX not sure if this actually useful, maybe just readyReadMessage...
/** Emitted when an end-of-stream marker arrives from the peer,
 * indicating that all remaining data is now queued for reading.
 * As with readyReadMessage(),
 * if the client delays reading data until receiving this signal,
 * it must ensure that the maximum receive window is large enough
 * or else monitor and respond to the receiveBlocked() signal.
 */
void readyReadComplete();
#endif

/** Emitted when we receive an incoming substream while listening.
 * In response the client should call acceptSubstream() in a loop
 * to accept all queued incoming substreams,
 * until acceptSubstream() returns NULL. */
void newSubstream();

/** Emitted when a queued incoming substream may be read as a datagram.
 * This occurs once the substream's entire data content arrives
 * and the remote peer closes its end while the substream is queued,
 * so that the entire content may be read at once via readDatagram().
 * (XXX or perhaps when an entire first message/record arrives?)
 * Note that if the client wishes to read datagrams using this signal,
 * the client must ensure that the parent's maximum receive window
 * is large enough to hold any incoming datagram that might arrive,
 * or else monitor the parent stream's receiveBlocked() signal
 * and grow the receive window to accommodate large datagrams.
 */
void readyReadDatagram();

/** Emitted when some locally buffered data gets flushed
 * after being delivered to the receiver and acknowledged.
 * XXX not implemented yet. */
void bytesWritten(qint64 bytes);

/** Emitted when the stream establishes live connectivity
 * upon first connecting, or after being down or stalled. */
void linkUp();

/** Emitted when connectivity on the stream has temporarily stalled.
 * SST emits the linkStalled() signal at the first sign of trouble:
 * this provides an early warning that the link may have failed,
 * but it may also just represent an ephemeral network glitch.
 * The application may wish to use this signal
 * to indicate the network status to the user. */
//void linkStalled();	XX implement

/** Emitted when link connectivity for the stream has been lost.
 * SST may emit this signal either due to a timeout
 * or due to detection of a link- or network-level "hard" failure.
 * The link may come back up sometime later, however,
 * in which case SST emits linkUp() and stream connectivity resumes.
 *
 * If the application desires TCP-like behavior
 * where a connection timeout causes permanent stream failure,
 * the application may simply destroy the stream
 * upon receiving the linkDown() signal. */
void linkDown();

void linkStatusChanged(LinkStatus status);

/** Emitted when incoming data has filled our receive window.
 * When this situation occurs, the client must read some queued data
 * or else increase the maximum receive window
 * before SST will accept further incoming data from the peer.
 * Every single byte of the receive window might not be utilized
 * when the receive process becomes blocked in this way,
 * because SST does not fragment packets just to "top up"
 * a nearly empty receive window: the effective limit
 * may be as low as half the specified maximum window size. */
void receiveBlocked();

/** Emitted when the stream is reset by either endpoint. */
void reset(const QString &errorString);

/** Emitted when an error condition is detected on the stream.
 *  Link stalls or failures are not considered error conditions. */
void error(const QString &errorString);


protected:
// Set an error condition on this Stream and emit the error() signal.
void setError(const QString &errorString);

// We override this method to watch our linkStatusChanged() signal.
void connectNotify(const char *signal);

private:
// Private constructor used internally
// to create a Stream wrapper for an existing BaseStream.
Stream(AbstractStream *as, QObject *parent);

// Connect the linkStatusNotify signal to the current peer's -
// we do this lazily so that if the app has many streams,
// not all of them necessarily have to be watching the peer.
void connectLinkStatusChanged();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Stream::ShutdownMode)


/** This class represents a server that can accept incoming SST connections.
 * To use this class,
 * the application creates a StreamServer instance,
 * calls listen() to begin listening for connections,
 * and upon arrival of a newConnection() signal
 * uses accept() to accept any queued incoming connections.
 */
class StreamServer : public QObject, public StreamProtocol
{
	friend class BaseStream;
	Q_OBJECT

private:
	Host *const h;			// Our per-host state
	QQueue<BaseStream*> rconns;	// Received connection stream queue
	QString svname;			// Service name
	QString svdesc;			// Longer service description
	QString prname;			// Protocol name
	QString prdesc;			// Longer protocol description
	QString err;
	bool active;


public:
	/** Create a StreamServer instance.
	 * The application must call listen()
	 * before the StreamServer will actually accept incoming connections.
	 *
	 * @param host the Host object containing hostwide SST state.
	 * @param parent the Qt parent for the new object.
	 *		The Stream will be automatically destroyed
	 *		when its parent is destroyed.
	 */
	StreamServer(Host *host, QObject *parent = NULL);

	/** Listen for incoming connections to a particular service
	 * using a particular application protocol.
	 * This method may only be called once on a StreamServer instance.
	 * An error occurs if another StreamServer object is already listening
	 * on the specified service/protocol name pair on this host.
	 * @param serviceName the service name on which to listen.
	 * 		Clients must specify the same service name
	 *		via connectTo() to connect to this server.
	 * @param serviceDescr a short human-readable service description
	 *		for use for example by utilities that
	 *		browse or control the set of services running
	 *		on a particular host.
	 * @param protocolName the protocol name on which to listen.
	 *		Clients must specify the same protocol name
	 *		via connectTo() to connect to this server.
	 * @param protocolDesc a short human-readable description
	 *		of the protocol, useful for browsing
	 *		the set of protocols a particular service supports.
	 * @return true if successful, false if an error occurred.
	 */
	bool listen(const QString &serviceName, const QString &serviceDesc,
		const QString &protocolName, const QString &protocolDesc);

	/// Returns true if this StreamServer is currently listening.
	inline bool isListening() { return active; }

	/** Accept an incoming connection as a top-level Stream.
	 * Upon receiving a newConnection() signal,
	 * the application must call accept() in a loop
	 * until there are no more incoming connections to accept.
	 *
	 * Stream objects returned from this method
	 * initially have the StreamServer as their Qt parent,
	 * so they are automatically deleted if the StreamServer is deleted.
	 * The application may re-parent these Stream objects if desired,
	 * in which case the Stream may outlive the StreamServer object.
	 *
	 * @return a new Stream representing ths incoming connection,
	 * 		or NULL if no connections are currently waiting.
	 */
	Stream *accept();

	/** Accept an incoming connection,
	 * and obtain the EID of the originating host.
	 * @overload
	 */
	inline Stream *accept(QByteArray &originHostId) {
		Stream *strm = accept();
		if (strm) originHostId = strm->remoteHostId();
		return strm; }

	/// Returns the service name previously supplied to listen().
	inline QString serviceName() { return svname; }

	/// Returns the service description previously supplied to listen().
	inline QString serviceDescription() { return svdesc; }

	/// Returns the protocol name previously supplied to listen().
	inline QString protocolName() { return prname; }

	/// Returns the protocol description previously supplied to listen().
	inline QString protocolDescription() { return prdesc; }

	/// Returns a string describing the last error that occurred, if any.
	inline QString errorString() { return err; }

signals:
	/// Emitted when a new connection arrives.
	void newConnection();

protected:
	/// Set the current error description string.
	inline void setErrorString(const QString &err) { this->err = err; }
};


// Private helper class,
// to register with Socket layer to receive key exchange packets.
// Only one instance ever created per host.
class StreamResponder : public KeyResponder, public StreamProtocol
{
	friend class StreamPeer;
	friend class StreamServer;
	friend class StreamHostState;
	Q_OBJECT

	StreamResponder(Host *h);

	// Set of RegClients we've connected to so far
	QPointerSet<RegClient> connrcs;


	void conncli(RegClient *cli);

	virtual Flow *newFlow(const SocketEndpoint &epi, const QByteArray &idi,
				const QByteArray &ulpi, QByteArray &ulpr);

private slots:
	void clientCreate(RegClient *rc);
	void clientStateChanged();
	void lookupNotify(const QByteArray &id, const Endpoint &loc,
			const RegInfo &info);
};


// Per-host state for the Stream module.
class StreamHostState : public QObject
{
	friend class BaseStream;
	friend class StreamServer;
	friend class StreamPeer;
	friend class StreamResponder;
	Q_OBJECT

private:
	StreamResponder *rpndr;
	QHash<StreamProtocol::ServicePair,StreamServer*> listeners;
	QHash<QByteArray,StreamPeer*> peers;


	StreamResponder *streamResponder();

public:
	inline StreamHostState() : rpndr(NULL) { }
	virtual ~StreamHostState();

	StreamPeer *streamPeer(const QByteArray &id, bool create = true);

	virtual Host *host() = 0;
};

} // namespace SST

#endif	/* SST_C_STREAM_H */
