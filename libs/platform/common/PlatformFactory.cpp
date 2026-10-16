#include <kstack/platform/IClipboardMonitor.hpp>

#ifndef __APPLE__
#include "QtClipboardMonitor.hpp"
#endif

#include <memory>

namespace kstack::platform {

#ifdef __APPLE__
std::unique_ptr<IClipboardMonitor> createMacClipboardMonitor();
#endif

std::unique_ptr<IClipboardMonitor> createClipboardMonitor() {
#ifdef __APPLE__
    return createMacClipboardMonitor();
#else
    return std::make_unique<QtClipboardMonitor>();
#endif
}

} // namespace kstack::platform
