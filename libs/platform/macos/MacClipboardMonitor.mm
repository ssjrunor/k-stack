/*
  Author: Runor Ewhro
  Description: observes macOS pasteboard changes while the app runs in the
               background and forwards supported text to the capture service.
*/
#include "MacClipboardMonitor.hpp"

#import <AppKit/AppKit.h>
#import <dispatch/dispatch.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace kstack::platform {

class MacClipboardMonitor final : public IClipboardMonitor {
public:
    MacClipboardMonitor(NSPasteboard* pasteboard, std::chrono::milliseconds interval)
        : pasteboard_(pasteboard), interval_(interval) {}

    ~MacClipboardMonitor() override { stop(); }

    // start from the current ownership version, so old clipboard contents are
    // not treated as a new copy when capture is enabled or resumed
    [[nodiscard]] bool start(TextCallback onText) override {
        stop();
        if (!pasteboard_) {
            return false;
        }
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150400
        if (@available(macOS 15.4, *)) {
            if (pasteboard_.accessBehavior == NSPasteboardAccessBehaviorAlwaysDeny) {
                return false;
            }
        }
#endif
        state_ = std::make_shared<State>();
        state_->callback = std::move(onText);
        state_->pasteboard = pasteboard_;
        state_->lastChangeCount = pasteboard_.changeCount;
        timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        if (!timer_) {
            state_.reset();
            return false;
        }
        auto state = state_;
        dispatch_source_set_event_handler(timer_, ^{ poll(state); });
        const auto period = std::max<std::int64_t>(interval_.count(), 1) * NSEC_PER_MSEC;
        dispatch_source_set_timer(timer_, dispatch_time(DISPATCH_TIME_NOW, period), period,
                                  period / 5);
        dispatch_resume(timer_);
        return true;
    }

    // clear the callback before canceling the timer, since an already queued
    // tick can still run after cancelation
    void stop() override {
        if (state_) {
            state_->callback = {};
        }
        if (timer_) {
            dispatch_source_cancel(timer_);
            timer_ = nullptr;
        }
        state_.reset();
    }

private:
    struct State {
        TextCallback callback;
        NSPasteboard* pasteboard = nil;
        NSInteger lastChangeCount = 0;
    };

    // read only after the ownership version changes; this avoids repeated
    // pasteboard reads and duplicate prompts during idle polling
    static void poll(const std::shared_ptr<State>& state) {
        @autoreleasepool {
            NSPasteboard* pasteboard = state->pasteboard;
            const NSInteger changeCount = pasteboard.changeCount;
            if (changeCount == state->lastChangeCount) {
                return;
            }
            state->lastChangeCount = changeCount;
            NSString* value = [pasteboard stringForType:NSPasteboardTypeString];
            if (!value) {
                value = [pasteboard stringForType:NSPasteboardTypeURL];
            }
            const char* utf8 = value.UTF8String;
            if (utf8 && *utf8 && state->callback) {
                state->callback(std::string(utf8));
            }
        }
    }

    NSPasteboard* pasteboard_ = nil;
    std::chrono::milliseconds interval_;
    std::shared_ptr<State> state_;
    dispatch_source_t timer_ = nullptr;
};

std::unique_ptr<IClipboardMonitor> createMacClipboardMonitor() {
    return std::make_unique<MacClipboardMonitor>([NSPasteboard generalPasteboard],
                                                 std::chrono::milliseconds(250));
}

std::unique_ptr<IClipboardMonitor> createMacClipboardMonitorForPasteboard(
    std::string_view name, std::chrono::milliseconds interval) {
    const std::string ownedName(name);
    NSString* pasteboardName = [NSString stringWithUTF8String:ownedName.c_str()];
    return std::make_unique<MacClipboardMonitor>([NSPasteboard pasteboardWithName:pasteboardName],
                                                 interval);
}

} // namespace kstack::platform
