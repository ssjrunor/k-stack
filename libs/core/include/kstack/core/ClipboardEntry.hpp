/*
 * Author: Runor Ewhro
 * Description: holds captured content and versioned presentation metadata.
 */
#pragma once

#include <kstack/core/ContentKind.hpp>

#include <chrono>
#include <cstdint>
#include <string>

namespace kstack::core {

struct ClipboardEntry {
    std::uint64_t id = 0;
    std::string text;
    std::chrono::system_clock::time_point capturedAt{};
    bool pinned = false;
    // recognised once at capture so every view agrees on what the text is
    ContentKind kind = ContentKind::Text;
    // for code, the language's name, such as "Python"; empty when the
    // language is unclear or the entry is not code
    std::string codeLanguage;
    // Stored classifications can be rebuilt after their rules change.
    bool metadataReady = true;
};

inline constexpr int kEntryClassificationVersion = 2;

} // namespace kstack::core
