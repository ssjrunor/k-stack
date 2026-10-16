#pragma once

namespace kstack::platform {

class IStartupManager {
public:
    virtual ~IStartupManager() = default;
    [[nodiscard]] virtual bool isEnabled() const = 0;
    virtual bool setEnabled(bool enabled) = 0;
};

} // namespace kstack::platform
