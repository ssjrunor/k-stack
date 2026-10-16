/*
 * Author: Runor Ewhro
 * Description: guesses which language a piece of text is written in, on the
 *              device, so only text in another language is translated.
 */
#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace kstack::translation {

struct DetectedLanguage {
    std::string language; // primary subtag, e.g. "de"
    // false when the text was too short or mixed for a confident answer
    bool reliable = false;
};

// UTF-8 plain text in; nothing when no language could be recognised or
// detection is not built into this copy of the app
[[nodiscard]] std::optional<DetectedLanguage> detectLanguage(std::string_view text);

} // namespace kstack::translation
