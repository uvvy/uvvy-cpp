#pragma once

class RtAudio;
class audio_sender; // @todo Remove
class audio_receiver; // @todo Remove
class abstract_audio_input;
class abstract_audio_output;

/**
 * Controls the audio layer and starts playback and capture.
 */
class audio_hardware
{
    RtAudio* audio_inst{0};
    audio_sender* sender_{0};
    audio_receiver* receiver_{0};

public:
    audio_hardware(audio_sender* sender, audio_receiver* receiver);
    ~audio_hardware();

    static bool add_instream(abstract_audio_input* in);
    static bool remove_instream(abstract_audio_input* in);
    static bool add_outstream(abstract_audio_output* out);
    static bool remove_outstream(abstract_audio_output* out);

    void open_audio();
    void close_audio();

    void start_audio();
    void stop_audio();

    void new_connection(shared_ptr<server> server,
        std::function<void(void)> on_start,
        std::function<void(void)> on_stop);

    void streaming(shared_ptr<stream> stream);
};
