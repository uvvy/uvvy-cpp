/**
 * @todo
 * Replace with a normal audio session setup here. Need to provide out of band signaling,
 * setting up a call session, session running status and session termination.
 *
 * VoiceService streams consist of a signaling substream and media substreams. Signaling substream has 
 * highest priority and reliable delivery. It allows negotiation and session control.
 * Media substreams are datagram-oriented and geared towards real-time audio (or video) traffic.
 * Establishing a session between two nodes consists of negotiating codec and bandwidth parameters
 * over signaling substream. After that the session is considered live and media substreams start.
 */
/**
 * Subclass of PeerService for providing voice communication.
 */
class VoiceService : public PeerService
{
    Q_OBJECT

private:
    /**
     * Minimum number of frames to queue before enabling output
     */
    static const int queueMin = 10; // 1/5 sec

    /**
     * Maximum number of frames to queue before dropping frames
     */
    static const int queueMax = 25; // 1/2 sec


    struct SendStream {
        Stream *stream;
        qint32 seqno;

        inline SendStream()
            : stream(NULL), seqno(0) { }
        inline SendStream(const SendStream &o)
            : stream(o.stream), seqno(o.seqno) { }
        inline SendStream(Stream *stream)
            : stream(stream), seqno(0) { }
    };

    struct ReceiveStream {
        Stream *stream;
        PacketOutput *vout;
        qint32 seqno;

        inline ReceiveStream()
            : stream(NULL), vout(NULL), seqno(0) { }
        inline ReceiveStream(const ReceiveStream &o)
            : stream(o.stream), vout(o.vout), seqno(o.seqno) { }
        inline ReceiveStream(Stream *stream, PacketOutput *vout)
            : stream(stream), vout(vout), seqno(0) { }
    };

    // Voice communication state
    PacketInput* vin;
    QSet<Stream*> sending;
    QHash<Stream*, SendStream> send;
    QHash<Stream*, ReceiveStream> recv;

    // Talk/listen status column configuration
    int talkcol, lisncol;
    QVariant talkena, talkdis, talkoff;
    QVariant lisnena, lisndis;

public:
    VoiceService(QObject *parent = NULL);

    // Control whether we're talking and/or listening to a given peer.
    void setTalkEnabled(const SST::PeerId &hostid, bool enable);
    void setListenEnabled(const SST::PeerId &hostid, bool enable);
    inline bool talkEnabled(const SST::PeerId &hostid)
        { return sending.contains(outStream(hostid)); }
    inline bool listenEnabled(const SST::PeerId &)
        { return true; /*XXX*/ }
    inline void toggleTalkEnabled(const SST::PeerId &hostid)
        { setTalkEnabled(hostid, !talkEnabled(hostid)); }
    inline void toggleListenEnabled(const SST::PeerId &hostid)
        { setListenEnabled(hostid, !listenEnabled(hostid)); }

    // Set up to display our status in particular PeerTable columns.
    void setTalkColumn(int column,
            const QVariant &enableValue = QVariant(),
            const QVariant &disableValue = QVariant(),
            const QVariant &offlineValue = QVariant());
    void setListenColumn(int column,
            const QVariant &enableValue = QVariant(),
            const QVariant &disableValue = QVariant());
    inline void clearTalkColumn() { setTalkColumn(-1); }
    inline void clearListenColumn() { setTalkColumn(-1); }

protected:
    /**
     * Override PeerService::updateStatus()
     * to provide additional talk/listen status indicators.
     */
    virtual void updateStatus(const SST::PeerId &id);

private slots:
    void gotOutStreamDisconnected(Stream *strm);
    void gotInStreamConnected(Stream *strm);
    void gotInStreamDisconnected(Stream *strm);
    void vinReadyRead();
    void voutQueueEmpty();
    void readyReadDatagram();
};
