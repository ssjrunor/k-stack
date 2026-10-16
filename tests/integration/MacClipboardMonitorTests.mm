/*
  Author: Runor Ewhro
  Description: verifies background text and URL capture from a private macOS
               pasteboard, including duplicate copies and source-process exit.
*/
#include "MacClipboardMonitor.hpp"

#include <kstack/core/CapturePipeline.hpp>
#include <kstack/core/ClipboardService.hpp>
#include <kstack/core/HistoryService.hpp>

#import <AppKit/AppKit.h>
#import <CoreFoundation/CoreFoundation.h>

#include <chrono>
#include <crt_externs.h>
#include <cstdlib>
#include <iostream>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

// the monitor runs on the main queue, so keep the main run loop active until
// an expected observation arrives or the short test timeout expires
template <typename Predicate> bool waitUntil(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!predicate() && std::chrono::steady_clock::now() < deadline) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, false);
    }
    return predicate();
}

struct PrivatePasteboard {
    NSPasteboard* value = [NSPasteboard pasteboardWithUniqueName];
    ~PrivatePasteboard() { [value releaseGlobally]; }
};

// write from a separate process and exit before the parent observes the
// change; this exercises ownership transfer without using the real clipboard
int writeFromChild(const std::string& name, const std::string& text, const char* executable) {
    pid_t child = 0;
    char* args[] = {const_cast<char*>(executable), const_cast<char*>("--write"),
                    const_cast<char*>(name.c_str()), const_cast<char*>(text.c_str()), nullptr};
    const int spawnResult = posix_spawn(&child, executable, nullptr, nullptr, args, *_NSGetEnviron());
    if (spawnResult != 0) {
        return spawnResult;
    }
    int status = 0;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status)) {
        return -1;
    }
    return WEXITSTATUS(status);
}

} // namespace

int main(int argc, char* argv[]) {
    @autoreleasepool {
        if (argc == 4 && std::string(argv[1]) == "--write") {
            NSString* name = [NSString stringWithUTF8String:argv[2]];
            NSPasteboard* pasteboard = [NSPasteboard pasteboardWithName:name];
            [pasteboard clearContents];
            return [pasteboard setString:[NSString stringWithUTF8String:argv[3]]
                                forType:NSPasteboardTypeString]
                       ? 0
                       : 1;
        }

        PrivatePasteboard pasteboard;
        if (!pasteboard.value) {
            std::cerr << "macOS pasteboard service is unavailable in this session\n";
            return 77;
        }
        const std::string name(pasteboard.value.name.UTF8String);
        kstack::core::HistoryService history;
        kstack::core::ClipboardService clipboard(history);
        kstack::core::CapturePipeline pipeline(clipboard);
        std::vector<std::string> notifications;
        auto monitor = kstack::platform::createMacClipboardMonitorForPasteboard(
            name, std::chrono::milliseconds(10));
        require(monitor->start([&](std::string text) {
            notifications.push_back(text);
            static_cast<void>(pipeline.processText(text));
        }), "private-pasteboard monitor could not start");

        const auto beforeCopy = std::chrono::system_clock::now();
        [pasteboard.value clearContents];
        require([pasteboard.value setString:@"first text" forType:NSPasteboardTypeString],
                "could not write text to private pasteboard");
        require(waitUntil([&] { return notifications.size() == 1; }),
                "background text change was not observed");
        require(notifications.front() == "first text", "captured text differs from pasteboard");
        require(history.entries().size() == 1, "first text was not recorded");
        require(history.entries().front().capturedAt >= beforeCopy,
                "capture timestamp precedes the copy");

        [pasteboard.value clearContents];
        require([pasteboard.value setString:@"first text" forType:NSPasteboardTypeString],
                "could not repeat private pasteboard text");
        require(waitUntil([&] { return notifications.size() == 2; }),
                "duplicate pasteboard notification was not observed");
        require(history.entries().size() == 1, "duplicate text entered history twice");

        [pasteboard.value clearContents];
        require([pasteboard.value setString:@"https://example.com/path"
                                    forType:NSPasteboardTypeURL],
                "could not write URL to private pasteboard");
        require(waitUntil([&] { return notifications.size() == 3; }),
                "URL-only pasteboard change was not observed");
        require(history.entries().front().text == "https://example.com/path",
                "URL content was not recorded");

        const std::string childText = "copied by a process that exited";
        require(writeFromChild(name, childText, argv[0]) == 0,
                "source process could not write its pasteboard item");
        require(waitUntil([&] { return notifications.size() == 4; }),
                "copy from an exited source process was not observed");
        require(history.entries().front().text == childText,
                "source-process text differs from captured history");

        monitor->stop();
        [pasteboard.value clearContents];
        require([pasteboard.value setString:@"while paused" forType:NSPasteboardTypeString],
                "could not write paused test text");
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
        require(notifications.size() == 4, "monitor captured text while stopped");
        std::cout << "macOS private-pasteboard capture checks passed\n";
    }
}
