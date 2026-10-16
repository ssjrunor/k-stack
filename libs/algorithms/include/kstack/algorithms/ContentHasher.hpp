/*
 * Author: Runor Ewhro
 * Description: supplies a fast fingerprint that always needs exact comparison.
 */
#pragma once

#include <cstdint>
#include <string_view>

namespace kstack::algorithms {

// A fast, non-cryptographic fingerprint. Stored hashes belong only inside an encrypted database.
std::uint64_t contentHash(std::string_view content) noexcept;

} // namespace kstack::algorithms
