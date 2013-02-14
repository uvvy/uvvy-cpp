#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QQueue>
#include <QMutex>

#include "RtAudio.h"

class QAbstractItemModel;
class AbstractAudioInput;
class AbstractAudioOutput;

/**
 * Audio I/O implementation using a portable RtAudio library.
 */
class Audio : public QObject
{
    friend class AbstractAudioInput;
    friend class AbstractAudioOutput;
    friend class AudioInputMonitor;
    Q_OBJECT

private:
    static int ndevs;
    static int indev, outdev;
    static QString indevname, outdevname;
    static QString errmsg;

    static QList<AbstractAudioInput*> instreams;
    static QList<AbstractAudioOutput*> outstreams;


    Audio();
    ~Audio();

public:
    static Audio *instance();

    // Initialize the audio system and scan for available devices.
    // Returns the number of devices available (may be 0), or -1 on error.
    // The error string is set appropriately on either 0 or -1 return.
    static int scan();

    // Re-scan to detect changes in the list of audio devices available.
    static int rescan();

    // Return the number of audio devices available and the name of each.
    // (Some may be input-only or output-only.)
    static inline int numDevices() { return ndevs; }
    static QString deviceName(int dev);
    static QStringList deviceNames();

    // Return models for the input and output device list
    static QAbstractItemModel *inputDeviceListModel();
    static QAbstractItemModel *outputDeviceListModel();

    // Find a device number by name, returning -1 if no such device exists.
    static int findDevice(const QString &name);

    // Get or set the currently selected input and output devices.
    static inline int inputDevice() { return indev; }
    static inline int outputDevice() { return outdev; }
    static void setInputDevice(int dev);
    static void setOutputDevice(int dev);
    static void setInputDevice(const QString &devname);
    static void setOutputDevice(const QString &devname);

    // Get the system-selected default input and output devices,
    // or -1 if no such device is available.
    static int defaultInputDevice();
    static int defaultOutputDevice();

    // Return number of input and output channels a given device supports
    static int inChannels(int dev);
    static int outChannels(int dev);

    // Return the minimum and maximum sample rates supported by a device
    static double minSampleRate(int dev);
    static double maxSampleRate(int dev);

    // Return the specific list of sample rates supported,
    // or the empty list if a continuous range is allowed.
    // This is for information only - the Audio class will (XXX should)
    // automatically re-sample to/from any desired rate.
    static QList<double> sampleRates(int dev);

    static inline QString errorString() { return instance()->errmsg; }

signals:
    // Signals for monitoring input/output level, measured in percent
    void inputLevelChanged(int level);
    void outputLevelChanged(int level);

protected:
    void connectNotify(const char *signal);
    void disconnectNotify(const char *signal);

private:
    static void open();
    static void reopen();
    static void close();

    static int rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData );
    static void sendin(const float *inbuf);
    static void mixout(float *outbuf);

    static int computeLevel(const float *buf);
    static void setInputLevel(int lev);
    static void setOutputLevel(int lev);
};

/**
 * Common base class for AudioInput and AudioOutput
 */
class AudioStream : public QObject
{
    Q_OBJECT

private:
    bool enab;
    int framesize;
    double rate;

public:
    AudioStream(QObject *parent);
    virtual ~AudioStream();

    // Enable or disable this stream.
    inline bool enabled() { return enab; }
    virtual void setEnabled(bool enable);
    inline void enable() { setEnabled(true); }
    inline void disable() { setEnabled(false); }

    // Get or set the frame size of this stream.
    // Frame size may only be changed while stream is disabled.
    inline int frameSize() { return framesize; }
    void setFrameSize(int framesize); 

    // Get or set the sample rate for this stream.
    // Sampling rate may only be changed while stream is disabled.
    inline double sampleRate() { return rate; }
    void setSampleRate(double samplerate);
};

/**
 * This abstract base class represents a source of audio input
 * from the currently selected input device, at a controllable bitrate.
 * Currently provides one-channel (Mono) audio only (appropriate for VoIP).
 */
class AbstractAudioInput : public AudioStream
{
    friend class Audio;
    Q_OBJECT

public:
    AbstractAudioInput(QObject *parent = NULL);
    AbstractAudioInput(int framesize, double samplerate,
            QObject *parent = NULL);

    void setEnabled(bool enabled);
    static const int nChannels = 1;

protected:
    // Accept audio input data received from the sound hardware.
    // This method is typically called from a dedicated audio thread,
    // so the subclass must handle multithread synchronization!
    virtual void acceptInput(const float *buf) = 0;
};

/**
 * This abstract base class represents a sink for audio output
 * to the currently selected output device, at a controllable bitrate.
 * Currently provides one-channel (Mono) audio only (appropriate for VoIP).
 */
class AbstractAudioOutput : public AudioStream
{
    friend class Audio;
    Q_OBJECT

public:
    AbstractAudioOutput(QObject *parent = NULL);
    AbstractAudioOutput(int framesize, double samplerate,
            QObject *parent = NULL);

    void setEnabled(bool enabled);
    static const int nChannels = 1;

protected:
    // Produce a frame of audio data to be sent to the sound hardware.
    // This method is typically called from a dedicated audio thread,
    // so the subclass must handle multithread synchronization!
    virtual void produceOutput(float *buf) = 0;

private:
    // Get output from client, resampling and/or buffering it as necessary
    // to match Audio::parate and Audio::paframesize.
    void getOutput(float *buf);
};

/**
 * This class represents a high-level source of audio input
 * from the currently selected input device, at a controllable bitrate,
 * providing automatic queueing and interthread synchronization.
 */
class AudioInput : public AbstractAudioInput
{
    Q_OBJECT

private:
    QMutex mutex;       // Protection for input queue
    QQueue<float*> inq; // Queue of audio input frames

public:
    AudioInput(QObject *parent = NULL);
    AudioInput(int framesize, double samplerate, QObject *parent = NULL);
    ~AudioInput();

    void setFrameSize(int framesize); 

    int readFramesAvailable();
    int readFrames(float *buf, int maxframes);
    QVector<float> readFrames(int maxframes = 1 << 30);

    // Disable the input stream and clear the input queue.
    void reset();

signals:
    void readyRead();

protected:
    // Accept audio input data received from the sound hardware.
    // The default implementation just adds the data to the input queue,
    // but this may be overridden to process the input data
    // directly in the context of the separate audio thread.
    // The subclass must handle multithread synchronization in this case!
    virtual void acceptInput(const float *buf);

private:
    void readInto(float *buf, int nframes);
};

/**
 * This class represents a high-level sink for audio output
 * to the currently selected output device, at a controllable bitrate,
 * providing automatic queueing and interthread synchronization.
 */
class AudioOutput : public AbstractAudioOutput
{
    friend class Audio;
    Q_OBJECT

private:
    QMutex mutex;       // Protection for output queue
    QQueue<float*> outq;    // Queue of audio output frames

public:
    AudioOutput(QObject *parent = NULL);
    AudioOutput(int framesize, double samplerate, QObject *parent = NULL);
    ~AudioOutput();

    void setFrameSize(int framesize);

    int numFramesQueued();
    int writeFrames(const float *buf, int nframes);
    int writeFrames(const QVector<float> &buf);

    // Disable the output stream and clear the output queue.
    void reset();

signals:
    void queueEmpty();

protected:
    // Produce a frame of audio data to be sent to the sound hardware.
    // The default implementation produces data from the output queue,
    // but it may be overridden to produce data some other way
    // directly in the context of the separate audio thread.
    // The subclass must handle multithread synchronization in this case!
    virtual void produceOutput(float *buf);
};

/**
 * Audio loopback - copies input to output with variable delay.
 */
class AudioLoop : public AudioStream
{
    Q_OBJECT

private:
    AudioInput in;
    AudioOutput out;
    float delay;
    int thresh;

public:
    AudioLoop(QObject *parent);

    // Loopback delay in seconds
    inline float loopDelay() { return delay; }
    void setLoopDelay(float secs);

    virtual void setEnabled(bool enable);

signals:
    void startPlayback();

private slots:
    void inReadyRead();
};

/**
 * Private helper class for input level monitoring.
 */
class AudioInputMonitor : public AudioInput
{
    friend class Audio;

private:
    AudioInputMonitor();

    virtual void acceptInput(const float *buf);
};
