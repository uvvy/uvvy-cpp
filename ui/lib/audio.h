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
 * Private helper class for input level monitoring.
 */
class AudioInputMonitor : public AudioInput
{
    friend class Audio;

private:
    AudioInputMonitor();

    virtual void acceptInput(const float *buf);
};
