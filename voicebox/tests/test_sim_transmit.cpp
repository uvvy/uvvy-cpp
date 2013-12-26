// Capture audio locally
// Send it through simulated network
// Playback locally again

#include "sim_host.h"
#include "sim_stream.h"
#include "sim_connection.h"
#include "sim_link.h"

rtaudio_source source_;
packet_sink sender_(&source_, sim_out_stream);
packet_source receiver_(sim_in_stream);
rtaudio_sink output_(&receiver_);
