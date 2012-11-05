#include "macsupport.h"

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

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
