#pragma once

#include <string>
#include <string_view>

namespace kstack::algorithms {

// ASCII case folding and outer whitespace trimming. Unicode folding is planned.
std::string normalizeText(std::string_view text);

} // namespace kstack::algorithms
