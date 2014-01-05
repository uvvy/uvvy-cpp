//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "macsupport.h"

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include <QWidget>

float dpiScaleFactor(QWidget* widget)
{
    NSView* view = reinterpret_cast<NSView*>(widget->winId());
    CGFloat scaleFactor = 1.0;
    if ([[view window] respondsToSelector: @selector(backingScaleFactor)])
        scaleFactor = [[view window] backingScaleFactor];

    return scaleFactor;
}

//=================================================================================================
// MacSupport
//=================================================================================================

static MacSupport *ms_instance = 0;

NSString * nsStringFromQString(const QString & s)
{
    const char * utf8String = s.toUtf8().constData();
    return [[NSString alloc] initWithUTF8String: utf8String];
}

void dockClickHandler(id self, SEL _cmd)
{
    Q_UNUSED(self);
    Q_UNUSED(_cmd);
    ms_instance->emitDockClick();
}

MacSupport::MacSupport() {
    Class cls = [[[NSApplication sharedApplication] delegate] class];

    if (!class_addMethod(cls, @selector(applicationShouldHandleReopen:hasVisibleWindows:), (IMP) dockClickHandler, "v@:"))
        NSLog(@"MyPrivate::MyPrivate() : class_addMethod failed!");

    ms_instance = this;
}

MacSupport::~MacSupport() {
}

void MacSupport::emitDockClick() {
    emit dockClicked();
}

void MacSupport::setDockBadge(const QString & badgeText)
{
    NSString * badgeString = nsStringFromQString(badgeText);
    [[NSApp dockTile] setBadgeLabel: badgeString];
    [badgeString release];
}

void MacSupport::requestAttention()
{
    [NSApp requestUserAttention: NSInformationalRequest];
}
