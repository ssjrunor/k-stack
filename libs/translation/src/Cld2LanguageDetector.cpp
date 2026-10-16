/*
 * Author: Runor Ewhro
 * Description: detects the language of captured text with CLD2, whose
 *              tables are compiled in, so detection needs no files or network.
 */
#include <kstack/translation/LanguageDetector.hpp>
#include <kstack/translation/LanguageTag.hpp>

#include <compact_lang_det.h>
#include <lang_script.h>

#include <climits>

namespace kstack::translation {

std::optional<DetectedLanguage> detectLanguage(std::string_view text) {
    if (text.empty() || text.size() > static_cast<std::size_t>(INT_MAX)) {
        return std::nullopt;
    }
    bool reliable = false;
    int validPrefix = 0;
    const CLD2::Language language = CLD2::DetectLanguageCheckUTF8(
        text.data(), static_cast<int>(text.size()), true, &reliable, &validPrefix);
    if (language == CLD2::UNKNOWN_LANGUAGE) {
        return std::nullopt;
    }
    std::string code = primaryLanguage(CLD2::LanguageCode(language));
    // CLD2 predates the current codes for Hebrew and Norwegian Bokmål
    if (code == "iw") {
        code = "he";
    } else if (code == "no") {
        code = "nb";
    }
    if (code.empty()) {
        return std::nullopt;
    }
    return DetectedLanguage{std::move(code), reliable};
}

} // namespace kstack::translation
