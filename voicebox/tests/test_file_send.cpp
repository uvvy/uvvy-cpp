#include <thread>
#include <chrono>
#include "host.h"
#include "stream.h"
#include "file_read_sink.h"
#include "opus_encode_sink.h"
#include "packet_sink.h"

using namespace std;
using namespace ssu;
using namespace voicebox;

int main()
{
    shared_ptr<host> host(host::create());
    shared_ptr<stream> stream(make_shared<stream>(host));

    file_read_sink readfile("bnb.s16");
    opus_encode_sink encoder;
    packet_sink sink(stream);

    encoder.set_producer(&readfile);
    sink.set_producer(&encoder);

    sink.set_frame_size(480);
    sink.set_sample_rate(48000);
    sink.set_num_channels(2);

    sink.enable();

    // @todo Need something pulling the sink to send actual packets. Some sort of send timer?

    this_thread::sleep_for(chrono::seconds(300));
}
