#pragma once

#include <cstddef>

namespace kstack::core {

class RetentionPolicy {
public:
    explicit RetentionPolicy(std::size_t maxEntries = 100) noexcept;
    [[nodiscard]] std::size_t maxEntries() const noexcept;

private:
    std::size_t maxEntries_;
};

} // namespace kstack::core
