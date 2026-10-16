#pragma once

#include <cstdint>
#include <string_view>

namespace kstack::algorithms {

// A fast, non-cryptographic fingerprint for in-memory comparison only.
std::uint64_t contentHash(std::string_view content) noexcept;

} // namespace kstack::algorithms
