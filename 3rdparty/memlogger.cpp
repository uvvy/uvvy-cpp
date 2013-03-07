#include "memlogger.h"

namespace CircularLogger
{
    Event g_events[BUFFER_SIZE];
    LONG g_pos = -1;
}
