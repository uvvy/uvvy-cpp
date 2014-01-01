//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

/**
 * Represents information about typed audio stream.
 * Keeps track of frame size, number of channels and sample rate.
 * The sample format is 32 bits float for convenience.
 * Frame size is number of samples in one frame. It does not take into account
 * number of channels.
 * Total size in bytes of one frame is frame_size() * num_channels() * sizeof(float).
 *
 * Common base class for audio_source and audio_sink.
 */
class audio_stream
{
    bool enabled_{false};
    unsigned int frame_size_{480};
    unsigned int num_channels_{1};
    double rate_{48000.0};

public:
    audio_stream() = default;
    audio_stream(int framesize, double samplerate, int channels = 1);
    virtual ~audio_stream();

    inline bool is_enabled() const { return enabled_; }
    /// Enable or disable this stream.
    virtual void set_enabled(bool enable);
    inline void enable() { set_enabled(true); }
    inline void disable() { set_enabled(false); }

    inline int frame_bytes() const { return frame_size_ * num_channels_ * sizeof(float); }

    /// Get or set the frame size of this stream.
    /// Frame size may only be changed while stream is disabled.
    inline unsigned int frame_size() const { return frame_size_; }
    virtual void set_frame_size(unsigned int frame_size);

    /// Get or set the sample rate for this stream.
    /// Sampling rate may only be changed while stream is disabled.
    inline double sample_rate() const { return rate_; }
    virtual void set_sample_rate(double rate);

    /// Get or set the number of channels for this stream.
    /// Number of channels may only be changed while stream is disabled.
    inline unsigned int num_channels() const { return num_channels_; }
    virtual void set_num_channels(unsigned int num_channels);
};
