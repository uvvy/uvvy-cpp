#ifndef VOICE_H
#define VOICE_H

#include <QMutex>
#include <QQueue>

#include <opus.h>

#include "audio.h"
#include "peer.h"

class OpusInput : public AbstractAudioInput
{
	Q_OBJECT

private:
	// Speex encoder state.
	// Owned exclusively by the audio thread while enabled.
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
	// Our implementation of AbstractAudioInput::acceptInput().
	virtual void acceptInput(const float *buf);
};

class OpusOutput : public AbstractAudioOutput
{
	Q_OBJECT

private:
	// Maximum number of consecutive frames to skip
	static const int maxSkip = 3;

	// Speex decoder state.
	// Owned exclusively by the audio thread while enabled.
	OpusDecoder *decstate;

	// Inter-thread synchronization and queueing state
	QMutex mutex;
	QQueue<QByteArray> outqueue;
	qint32 outseq;

public:
	OpusOutput(QObject *parent = NULL);

	void setEnabled(bool enabled);

	// Write a frame with the given seqno to the tail of the queue,
	// padding the queue as necessary to account for missed frames.
	// If the queue gets longer than queuemax, drop frames from the head.
	void writeFrame(const QByteArray &buf, qint32 seqno, int queuemax);

	// Return the current length of the output queue.
	int numFramesQueued();

	// Disable the stream and clear the output queue.
	void reset();

signals:
	void queueEmpty();

private:
	// Our implementation of AbstractAudioOutput::produceOutput().
	virtual void produceOutput(float *buf);
};


class VoiceService : public PeerService
{
	Q_OBJECT

private:
	// Minimum number of frames to queue before enabling output
	static const int queueMin = 10;	// 1/5 sec

	// Maximum number of frames to queue before dropping frames
	static const int queueMax = 25;	// 1/2 sec


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
	void setTalkEnabled(const QByteArray &hostid, bool enable);
	void setListenEnabled(const QByteArray &hostid, bool enable);
	inline bool talkEnabled(const QByteArray &hostid)
		{ return sending.contains(outStream(hostid)); }
	inline bool listenEnabled(const QByteArray &)
		{ return true; /*XXX*/ }
	inline void toggleTalkEnabled(const QByteArray &hostid)
		{ setTalkEnabled(hostid, !talkEnabled(hostid)); }
	inline void toggleListenEnabled(const QByteArray &hostid)
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
	// Override PeerService::updateStatus()
	// to provide additional talk/listen status indicators.
	virtual void updateStatus(const QByteArray &id);

private slots:
	void gotOutStreamDisconnected(Stream *strm);
	void gotInStreamConnected(Stream *strm);
	void gotInStreamDisconnected(Stream *strm);
	void vinReadyRead();
	void voutQueueEmpty();
	void readyReadDatagram();
};

#endif	// VOICE_H
