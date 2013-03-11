#pragma once

namespace ssu {

class abstract_stream;

/**
 * User-space interface to the stream.
 * This is the primary high-level class that client applications use to communicate over the network via SSU.
 * The class provides standard stream-oriented read/write functionality via the methods in its QIODevice base class,
 * and adds SSU-specific methods for controlling SSU streams and substreams.
 *
 * To initiate an outgoing "top-level" SSU stream to a remote host, the client application creates a stream instance
 * and then calls connect_to().
 * To initiate a sub-stream from an existing stream, the application calls open_substream() on the parent stream.
 *
 * To accept incoming top-level streams from other hosts the application creates a ssu::server instance,
 * and that class creates stream instances for incoming connections.
 * To accept new incoming substreams on existing streams, the application calls listen() on the parent stream,
 * and upon arrival of a new_substream() signal the application calls accept_substream()
 * to obtain a stream object for the new incoming substream.
 *
 * SSU uses service and protocol names in place of the port numbers used by TCP and UDP to differentiate and select
 * among different application protocols.
 * A service name represents an abstract service being provided: e.g., "Web", "File", "E-mail", etc.
 * A protocol name represents a concrete application protocol to be used for communication with an abstract service:
 * e.g., "HTTP 1.0" or "HTTP 1.1" for communication with a "Web" service; "FTP", "NFS v4", or "CIFS" for communication 
 * with a "File" service; "SMTP", "POP3", or "IMAP4" for communication with an "E-mail" service.
 * Service names are intended to be suitable for non-technical users to see, in a service manager or firewall configuration 
 * utility for example, while protocol names are primarily intended for application developers.
 * A server can support multiple distinct protocols on one logical service, for backward compatibility or functional modularity
 * reasons for example, by registering to listen on multiple (service, protocol) name pairs.
 *
 * @see server
 */
class stream
{
    abstract_stream* stream_;

public:
    typedef uint16_t id_t;      ///< Stream ID within channel.
    typedef uint32_t byteseq_t; ///< Stream byte sequence number.
    typedef uint64_t counter_t; ///< Counter for SID assignment.

    /**
     * Flag bits used as arguments to the listen() method, indicating when and how to accept incoming substreams.
     */
    enum class listen_mode
    {
        reject         = 0,    ///< Reject incoming substreams.
        buffer_limit   = 2,    ///< Accept subs up to receive buffer size.
        unlimited      = 3,    ///< Accept substreams of any size.
        inherit        = 4,    ///< Flag: Substreams inherit this listen mode.
    };

    /**
     * Flag bits used as operands to the shutdown() method, indicating how and in which direction(s) to shutdown a stream.
     */
    enum class shutdown_mode
    {
        read    = 1,    ///< Read (incoming data) direction.
        write   = 2,    ///< Write (outgoing data) direction.
        close   = 3,    ///< Both directions (Read|Write).
        reset   = 4,    ///< Forceful reset.
    };

    /**
     * Type for identifying streams uniquely across channels.
     *
     * XXX should contain a "keying method identifier" of some kind?
     */
    struct unique_stream_id_t
    {
        counter_t counter; ///< Stream counter in channel
        std::vector<char> half_channel_id; ///< Unique channel+direction ID ("half-channel id")
    };
};

} // namespace ssu
