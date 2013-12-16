#include <thread>
#include <chrono>
#include "file_read_sink.h"
#include "rtaudio_sink.h"

using namespace std;
using namespace voicebox;

int main()
{
    file_read_sink readfile("bnb.s16");
    rtaudio_sink sink;
    sink.set_producer(&readfile);

    sink.enable();

    this_thread::sleep_for(chrono::seconds(30));
}
