/*
  Author: Runor Ewhro
  Description: declares the private-pasteboard factory used to exercise the
               macOS monitor without touching the user's clipboard.
*/
#pragma once

#include <kstack/platform/IClipboardMonitor.hpp>

#include <chrono>
#include <memory>
#include <string_view>

namespace kstack::platform {

std::unique_ptr<IClipboardMonitor> createMacClipboardMonitorForPasteboard(
    std::string_view name, std::chrono::milliseconds interval);

} // namespace kstack::platform
