// from http://preshing.com/20120522/lightweight-in-memory-logging
#pragma once

namespace CircularLogger
{
    struct Event
    {
        DWORD tid;        // Thread ID
        const char* msg;  // Message string
        DWORD param;      // A parameter which can mean anything you want
    };

    static const int BUFFER_SIZE = 65536;   // Must be a power of 2
    extern Event g_events[BUFFER_SIZE];
    extern LONG g_pos;

    inline void Log(const char* msg, DWORD param)
    {
        // Get next event index
        LONG index = _InterlockedIncrement(&g_pos);
        // Write an event at this index
        Event* e = g_events + (index & (BUFFER_SIZE - 1));  // Wrap to buffer size
        e->tid = ((DWORD*) __readfsdword(24))[9];           // Get thread ID
        e->msg = msg;
        e->param = param;
    }
}

#define TRACE(m, p) CircularLogger::Log(m, p)
