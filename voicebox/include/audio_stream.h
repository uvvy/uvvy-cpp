#pragma once

/**
 * Represents information about typed audio stream.
 * Keeps track of frame size, number of channels and sample rate.
 *
 * Common base class for audio_source and audio_sink.
 */
class audio_stream
{
    bool enabled_{false};
    int frame_size_{480};
    int num_channels_{1};
    double rate_{48000.0};

public:
    audio_stream() = default;
    virtual ~audio_stream();

    inline bool is_enabled() const { return enabled_; }
    /// Enable or disable this stream.
    virtual void set_enabled(bool enable);
    inline void enable() { set_enabled(true); }
    inline void disable() { set_enabled(false); }

    /// Get or set the frame size of this stream.
    /// Frame size may only be changed while stream is disabled.
    inline int frame_size() const { return frame_size_; }
    virtual void set_frame_size(int frame_size);

    /// Get or set the sample rate for this stream.
    /// Sampling rate may only be changed while stream is disabled.
    inline double sample_rate() const { return rate_; }
    virtual void set_sample_rate(double rate);

    /// Get or set the number of channels for this stream.
    /// Number of channels may only be changed while stream is disabled.
    inline int num_channels() const { return num_channels_; }
    virtual void set_num_channels(int num_channels);
};
