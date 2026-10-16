/*
 * Author: Runor Ewhro
 * Description: reduces language tags such as "en-CA" to the language the
 *              translation models name, and picks the user's first preferred
 *              language that captured text can be translated into.
 */
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kstack::translation {

// the lowercase primary subtag: "en-CA" and "en_CA" become "en"; an empty or
// malformed tag gives an empty string
[[nodiscard]] std::string primaryLanguage(std::string_view tag);

// the first of the preferred tags, in order, whose language is accepted;
// duplicates such as "en-CA" then "en-US" are only tried once
[[nodiscard]] std::optional<std::string>
chooseTargetLanguage(const std::vector<std::string>& preferredTags,
                     const std::function<bool(const std::string&)>& canTranslateInto);

} // namespace kstack::translation
