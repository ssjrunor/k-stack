/*
  Author: Runor Ewhro
  Description: defines the platform boundary for observing supported clipboard
               additions without exposing operating-system APIs to the core.
*/
#pragma once

#include <functional>
#include <memory>
#include <string>

namespace kstack::platform {

class IClipboardMonitor {
public:
    using TextCallback = std::function<void(std::string)>;
    using AccessCallback = std::function<void(bool)>;
    virtual ~IClipboardMonitor() = default;
    // Reports changes in access while observation is running. A false start
    // still reports its initial state through the return value.
    [[nodiscard]] virtual bool start(TextCallback onText, AccessCallback onAccessChanged = {}) = 0;
    virtual void stop() = 0;
};

[[nodiscard]] std::unique_ptr<IClipboardMonitor> createClipboardMonitor();

} // namespace kstack::platform
