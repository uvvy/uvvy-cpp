#include <QWidget>

float dpiScaleFactor(QWidget* widget)
{
    NSView* view = reinterpret_cast<NSView*>(widget->winId());
    CGFloat scaleFactor = 1.0;
    if ([[view window] respondsToSelector: @selector(backingScaleFactor)])
        scaleFactor = [[view window] backingScaleFactor];

    return scaleFactor;
}
