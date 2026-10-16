#pragma once

#include <string_view>

namespace kstack::algorithms {

// Returns zero when there is no case-insensitive ASCII substring match.
int searchScore(std::string_view content, std::string_view query);

} // namespace kstack::algorithms
