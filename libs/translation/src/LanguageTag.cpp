/*
 * Author: Runor Ewhro
 * Description: reduces language tags to primary subtags and picks the first
 *              preferred language that has a translation route.
 */
#include <kstack/translation/LanguageTag.hpp>

#include <algorithm>

namespace kstack::translation {

std::string primaryLanguage(std::string_view tag) {
    const auto end = tag.find_first_of("-_");
    const std::string_view primary = tag.substr(0, end);
    // primary subtags are two or three letters; anything else is not a tag
    if (primary.size() < 2 || primary.size() > 3) {
        return {};
    }
    std::string result;
    for (const char c : primary) {
        if (c >= 'A' && c <= 'Z') {
            result.push_back(static_cast<char>(c - 'A' + 'a'));
        } else if (c >= 'a' && c <= 'z') {
            result.push_back(c);
        } else {
            return {};
        }
    }
    return result;
}

std::optional<std::string>
chooseTargetLanguage(const std::vector<std::string>& preferredTags,
                     const std::function<bool(const std::string&)>& canTranslateInto) {
    std::vector<std::string> tried;
    for (const auto& tag : preferredTags) {
        std::string language = primaryLanguage(tag);
        if (language.empty() || std::find(tried.begin(), tried.end(), language) != tried.end()) {
            continue;
        }
        if (canTranslateInto(language)) {
            return language;
        }
        tried.push_back(std::move(language));
    }
    return std::nullopt;
}

} // namespace kstack::translation
