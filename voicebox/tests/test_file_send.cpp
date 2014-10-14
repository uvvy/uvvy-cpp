#include <thread>
#include <chrono>
#include "sss/host.h"
#include "sss/stream.h"
#include "arsenal/settings_provider.h"
#include "voicebox/file_read_sink.h"
#include "voicebox/opus_encode_sink.h"
#include "voicebox/packet_sink.h"

using namespace std;
using namespace sss;
using namespace voicebox;

int main()
{
    shared_ptr<host> host(host::create(settings_provider::instance()));
    shared_ptr<stream> stream(make_shared<stream>(host));

    // stream->connect_to(eid, "streaming", "opus");//-- audio_service?

    file_read_sink readfile("bnb.s16");
    opus_encode_sink encoder;
    packet_sink sink(stream);

    encoder.set_producer(&readfile);
    sink.set_producer(&encoder);

    sink.set_frame_size(480);
    sink.set_sample_rate(48000);
    sink.set_num_channels(2);

    sink.enable();

    // Emulate 30 seconds of looped playback
    for (int i = 0; i < 3000; ++i) {
        byte_array buf;
        sink.produce_output(buf);
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}
