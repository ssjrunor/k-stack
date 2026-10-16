#pragma once

#include <functional>

namespace kstack::platform {

class IGlobalShortcut {
public:
    virtual ~IGlobalShortcut() = default;
    virtual bool registerSearchShortcut(std::function<void()> onActivated) = 0;
    virtual void unregisterSearchShortcut() = 0;
};

} // namespace kstack::platform
