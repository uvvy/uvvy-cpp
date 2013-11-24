//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <QWidget>

float dpiScaleFactor(QWidget* widget)
{
    NSView* view = reinterpret_cast<NSView*>(widget->winId());
    CGFloat scaleFactor = 1.0;
    if ([[view window] respondsToSelector: @selector(backingScaleFactor)])
        scaleFactor = [[view window] backingScaleFactor];

    return scaleFactor;
}
