#pragma once

#include <QMutex>
#include <QQueue>
#include <QFile>

#include <opus.h>

#include "audio.h"
#include "peer.h"

//=====================================================================================================================
// OpusInput
//=====================================================================================================================

/**
 * This class represents an Opus-encoded source of audio input,
 * providing automatic queueing and interthread synchronization.
 */
class OpusInput : public AbstractAudioInput
{
    Q_OBJECT

private:
    /**
     * Encoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusEncoder *encstate;

    // Inter-thread synchronization and queueing state
    QMutex mutex;
    QQueue<QByteArray> inqueue;

signals:
    void readyRead();

public:
    OpusInput(QObject *parent = NULL);

    void setEnabled(bool enabled);

    QByteArray readFrame();

private:
    /**
     * Our implementation of AbstractAudioInput::acceptInput().
     * @param buf [description]
     */
    virtual void acceptInput(const float *buf);
};

//=====================================================================================================================
// RawInput
//=====================================================================================================================

/**
 * This class represents a raw unencoded source of audio input,
 * providing automatic queueing and interthread synchronization.
 * This data is usually received from network, but could also be supplied from a file.
 * It uses an XDR-based encoding to simplify interoperation.
 */
class RawInput : public AbstractAudioInput
{
    Q_OBJECT

private:
    // Inter-thread synchronization and queueing state
    QMutex mutex;
    QQueue<QByteArray> inqueue;

signals:
    void readyRead();

public:
    RawInput(QObject *parent = NULL);

    QByteArray readFrame();

private:
    /**
     * Our implementation of AbstractAudioInput::acceptInput().
     * @param buf [description]
     */
    virtual void acceptInput(const float *buf);
};

//=====================================================================================================================
// OpusOutput
//=====================================================================================================================

/**
 * This class represents a high-level sink for audio output
 * to the currently selected output device, at a controllable bitrate,
 * providing automatic queueing and interthread synchronization.
 */
class OpusOutput : public AbstractAudioOutput
{
    Q_OBJECT

private:
    /**
     * Maximum number of consecutive frames to skip
     */
    static const int maxSkip = 3;

    /**
     * Decoder state.
     * Owned exclusively by the audio thread while enabled.
     */
    OpusDecoder *decstate;

    // Inter-thread synchronization and queueing state
    QMutex mutex;
    QQueue<QByteArray> outqueue;
    qint32 outseq;

public:
    OpusOutput(QObject *parent = NULL);

    void setEnabled(bool enabled);

    /**
     * Write a frame with the given seqno to the tail of the queue,
     * padding the queue as necessary to account for missed frames.
     * If the queue gets longer than queuemax, drop frames from the head.
     * @param buf      [description]
     * @param seqno    [description]
     * @param queuemax [description]
     */
    void writeFrame(const QByteArray &buf, qint32 seqno, int queuemax);

    /**
     * Return the current length of the output queue.
     */
    int numFramesQueued();

    /**
     * Disable the stream and clear the output queue.
     */
    void reset();

signals:
    void queueEmpty();

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf [description]
     */
    virtual void produceOutput(float *buf);
};

//=====================================================================================================================
// RawOutput
//=====================================================================================================================

class RawOutput : public AbstractAudioOutput
{
    Q_OBJECT

    // Inter-thread synchronization and queueing state
    QMutex mutex;
    QQueue<QByteArray> outqueue;
    qint32 outseq;

public:
    RawOutput(QObject *parent = NULL);

signals:
    void queueEmpty();

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf [description]
     */
    virtual void produceOutput(float *buf);
};

//=====================================================================================================================
// FileLoopedOutput
//=====================================================================================================================

/**
 * This class provides data to hardware output by reading a specified file in a loop.
 * It provides a local-only playback of a given file.
 * It can be used by signaling on both sides to play a call signal, for example.
 * If you want to send the file contents over the wire, use FileLoopedInput.
 *
 * Current implementation plays mono 16 bit raw audio file in an endless loop.
 */
class FileLoopedOutput : public AbstractAudioOutput
{
    Q_OBJECT

    QFile file;
    qint64 offset;

public:
    FileLoopedOutput(const QString& fileName, QObject *parent = NULL);

    void setEnabled(bool enabling);

    /**
     * Disable the stream and clear the output queue.
     */
    void reset();

signals:
    void queueEmpty(); ///< Never fired.

private:
    /**
     * Our implementation of AbstractAudioOutput::produceOutput().
     * @param buf Output hardware buffer to write data to. The size is hwframesize.
     */
    virtual void produceOutput(float *buf);
};

//=====================================================================================================================
// VoiceService
//=====================================================================================================================

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
        OpusOutput *vout;
        qint32 seqno;

        inline ReceiveStream()
            : stream(NULL), vout(NULL), seqno(0) { }
        inline ReceiveStream(const ReceiveStream &o)
            : stream(o.stream), vout(o.vout), seqno(o.seqno) { }
        inline ReceiveStream(Stream *stream, OpusOutput *vout)
            : stream(stream), vout(vout), seqno(0) { }
    };

    // Voice communication state
    OpusInput vin;
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
