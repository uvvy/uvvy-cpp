#pragma once

#include <boost/signals2/signal.hpp>
#include "audio_input.h"
#include "audio_output.h"

/**
 * Audio loopback - copies input to output with variable delay.
 */
class audio_loopback : public audio_stream
{
    typedef audio_stream super;

    audio_input in_;
    audio_output out_;
    float delay_{4.0};
    size_t threshold_{0};

public:
    audio_loopback();

    // Loopback delay in seconds
    inline float loopback_delay() const { return delay_; }
    void set_loopback_delay(float secs);

    void set_enabled(bool enabling) override;

    typedef boost::signals2::signal<void (void)> start_signal;
    start_signal on_start_playback;

private:
    void in_ready_read();
};

