/**
 * A Qt4-compatible wrapper around ssu::stream.
 * Forwards boost signals to Qt signals.
 */
#pragma once

#include "stream.h"

//QObject?
class Stream : public ssu::stream
{
    Q_OBJECT
};
