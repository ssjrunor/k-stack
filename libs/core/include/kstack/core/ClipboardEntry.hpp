#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace kstack::core {

struct ClipboardEntry {
    std::uint64_t id = 0;
    std::string text;
    std::chrono::system_clock::time_point capturedAt{};
    bool pinned = false;
};

} // namespace kstack::core
