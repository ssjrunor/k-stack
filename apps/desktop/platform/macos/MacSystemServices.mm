/*
 * Author: Runor Ewhro
 * Description: shows the macOS share menu for an entry, opens Calendar on a
 *              day through Apple Events, opens event files in Calendar so it
 *              asks which calendar to use, creates reminders through EventKit
 *              once the user grants access, and shows places in Maps.
 */
#include "MacSystemServices.hpp"

#import <AppKit/AppKit.h>
#import <EventKit/EventKit.h>

#include <QDateTime>
#include <QUrl>
#include <QUrlQuery>
#include <QWindow>

// reports when the user dismisses the share menu, or when the chosen service
// finishes, so the panel knows it may close again
@interface KStackShareDelegate : NSObject <NSSharingServicePickerDelegate, NSSharingServiceDelegate>
@property(nonatomic, copy) void (^onDone)(BOOL shared);
@property(nonatomic, weak) NSWindow* sourceWindow;
@end

@implementation KStackShareDelegate
- (void)sharingServicePicker:(NSSharingServicePicker*)picker
     didChooseSharingService:(NSSharingService*)service {
    (void)picker;
    if (!service && self.onDone) {
        self.onDone(NO);
    }
}
- (id<NSSharingServiceDelegate>)sharingServicePicker:(NSSharingServicePicker*)picker
                           delegateForSharingService:(NSSharingService*)service {
    (void)picker;
    (void)service;
    return self;
}
- (void)sharingService:(NSSharingService*)service didShareItems:(NSArray*)items {
    (void)service;
    (void)items;
    if (self.onDone) {
        self.onDone(YES);
    }
}
- (void)sharingService:(NSSharingService*)service
    didFailToShareItems:(NSArray*)items
                  error:(NSError*)error {
    (void)service;
    (void)items;
    (void)error;
    if (self.onDone) {
        self.onDone(NO);
    }
}
- (NSWindow*)sharingService:(NSSharingService*)service
    sourceWindowForShareItems:(NSArray*)items
          sharingContentScope:(NSSharingContentScope*)scope {
    (void)service;
    (void)items;
    (void)scope;
    return self.sourceWindow;
}
@end

namespace kstack::desktop {

struct MacSystemServices::Impl {
    EKEventStore* store = nil;
    NSSharingServicePicker* picker = nil;
    KStackShareDelegate* shareDelegate = nil;
};

namespace {
NSDateComponents* dueComponents(const QDateTime& when, bool allDay) {
    NSDateComponents* components = [[NSDateComponents alloc] init];
    components.calendar = NSCalendar.currentCalendar;
    components.year = when.date().year();
    components.month = when.date().month();
    components.day = when.date().day();
    if (!allDay) {
        components.hour = when.time().hour();
        components.minute = when.time().minute();
    }
    return components;
}
} // namespace

MacSystemServices::MacSystemServices(QObject* parent)
    : SystemServices(parent), impl_(std::make_unique<Impl>()) {}

MacSystemServices::~MacSystemServices() = default;

bool MacSystemServices::canShare() const { return true; }

bool MacSystemServices::canShowCalendarDay() const { return true; }

bool MacSystemServices::canAddReminder() const { return true; }

void MacSystemServices::share(const QString& text, const QString& url, qreal x, qreal y) {
    if (!window()) {
        return;
    }
    NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window()->winId());
    id item = url.isEmpty() ? nil : [NSURL URLWithString:url.toNSString()];
    if (!item) {
        item = text.toNSString();
    }

    impl_->shareDelegate = [[KStackShareDelegate alloc] init];
    impl_->shareDelegate.sourceWindow = view.window;
    impl_->shareDelegate.onDone = ^(BOOL shared) {
        setBusy(false);
        if (shared) {
            emit finished(QStringLiteral("Shared"), true);
        }
    };
    impl_->picker = [[NSSharingServicePicker alloc] initWithItems:@[ item ]];
    impl_->picker.delegate = impl_->shareDelegate;

    const CGFloat top = view.isFlipped ? y : view.bounds.size.height - y;
    setBusy(true);
    [impl_->picker showRelativeToRect:NSMakeRect(x, top, 1, 1)
                               ofView:view
                        preferredEdge:NSRectEdgeMinY];
}

// Calendar has no public URL for a date, so it is driven with Apple Events;
// the first use asks the user to allow controlling Calendar
void MacSystemServices::showCalendarDay(const QString& isoDateTime) {
    const QDateTime when = QDateTime::fromString(isoDateTime, Qt::ISODate);
    if (!when.isValid()) {
        return;
    }
    // the day is set to 1 first so changing the month never overflows
    const QString source = QStringLiteral(
        "set d to current date\n"
        "set day of d to 1\n"
        "set year of d to %1\n"
        "set month of d to %2\n"
        "set day of d to %3\n"
        "set time of d to %4\n"
        "tell application \"Calendar\"\n"
        "  activate\n"
        "  switch view to day view\n"
        "  view calendar at d\n"
        "end tell\n")
        .arg(when.date().year())
        .arg(when.date().month())
        .arg(when.date().day())
        .arg(when.time().msecsSinceStartOfDay() / 1000);
    NSString* script = source.toNSString();
    setBusy(true);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSDictionary* error = nil;
        NSAppleScript* apple = [[NSAppleScript alloc] initWithSource:script];
        [apple executeAndReturnError:&error];
        const NSInteger code = error ? [error[NSAppleScriptErrorNumber] integerValue] : 0;
        dispatch_async(dispatch_get_main_queue(), ^{
            setBusy(false);
            if (code == -1743) {
                emit finished(QStringLiteral("Allow control of Calendar in System Settings, Privacy & "
                                             "Security, Automation, then try again."),
                              false);
            } else if (code != 0) {
                emit finished(QStringLiteral("Calendar could not open that day."), false);
            }
        });
    });
}

// open the file in Calendar itself, whichever app handles calendar files
void MacSystemServices::openEventFile(const QString& path) {
    NSURL* fileUrl = [NSURL fileURLWithPath:path.toNSString()];
    NSURL* calendar = [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:@"com.apple.iCal"];
    if (!calendar) {
        SystemServices::openEventFile(path);
        return;
    }
    [NSWorkspace.sharedWorkspace openURLs:@[ fileUrl ]
                     withApplicationAtURL:calendar
                            configuration:[NSWorkspaceOpenConfiguration configuration]
                        completionHandler:nil];
}

void MacSystemServices::addReminder(const QString& title, const QString& isoDateTime, bool allDay) {
    const QDateTime when = QDateTime::fromString(isoDateTime, Qt::ISODate);
    if (!when.isValid()) {
        return;
    }
    if (!impl_->store) {
        impl_->store = [[EKEventStore alloc] init];
    }
    EKEventStore* store = impl_->store;
    NSString* name = (title.trimmed().isEmpty() ? QStringLiteral("Reminder") : title.trimmed()).toNSString();
    NSDateComponents* due = dueComponents(when, allDay);
    NSDate* alarmDate = when.toNSDate();

    setBusy(true);
    [store requestFullAccessToRemindersWithCompletion:^(BOOL granted, NSError* requestError) {
        (void)requestError;
        dispatch_async(dispatch_get_main_queue(), ^{
            setBusy(false);
            if (!granted) {
                emit finished(QStringLiteral("Allow Reminders access in System Settings, Privacy & "
                                             "Security, Reminders, then try again."),
                              false);
                return;
            }
            EKReminder* reminder = [EKReminder reminderWithEventStore:store];
            reminder.title = name;
            reminder.calendar = store.defaultCalendarForNewReminders;
            reminder.dueDateComponents = due;
            if (!allDay) {
                [reminder addAlarm:[EKAlarm alarmWithAbsoluteDate:alarmDate]];
            }
            NSError* saveError = nil;
            if (!reminder.calendar || ![store saveReminder:reminder commit:YES error:&saveError]) {
                emit finished(QStringLiteral("The reminder could not be saved."), false);
                return;
            }
            emit finished(QStringLiteral("Reminder added"), true);
        });
    }];
}

void MacSystemServices::showOnMap(const QString& place) {
    QUrl url(QStringLiteral("https://maps.apple.com/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("q"), place);
    url.setQuery(query);
    [NSWorkspace.sharedWorkspace openURL:url.toNSURL()];
}

} // namespace kstack::desktop
