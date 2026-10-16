#pragma once

#include <optional>
#include <string>

namespace kstack::platform {

class ISourceDetector {
public:
    virtual ~ISourceDetector() = default;
    [[nodiscard]] virtual std::optional<std::string> foregroundApplication() const = 0;
};

} // namespace kstack::platform
