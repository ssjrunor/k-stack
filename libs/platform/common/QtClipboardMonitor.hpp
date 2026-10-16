/*
  Author: Runor Ewhro
  Description: declares the Qt clipboard observer used on Windows and Linux.
*/
#pragma once

#include <kstack/platform/IClipboardMonitor.hpp>

#include <QMetaObject>

namespace kstack::platform {

class QtClipboardMonitor final : public IClipboardMonitor {
public:
    ~QtClipboardMonitor() override;
    [[nodiscard]] bool start(TextCallback onText) override;
    void stop() override;

private:
    QMetaObject::Connection connection_;
};

} // namespace kstack::platform
