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
    virtual ~IClipboardMonitor() = default;
    [[nodiscard]] virtual bool start(TextCallback onText) = 0;
    virtual void stop() = 0;
};

[[nodiscard]] std::unique_ptr<IClipboardMonitor> createClipboardMonitor();

} // namespace kstack::platform
