#include <kstack/core/RetentionPolicy.hpp>

namespace kstack::core {

RetentionPolicy::RetentionPolicy(std::size_t maxEntries) noexcept : maxEntries_(maxEntries) {}

std::size_t RetentionPolicy::maxEntries() const noexcept { return maxEntries_; }

} // namespace kstack::core
