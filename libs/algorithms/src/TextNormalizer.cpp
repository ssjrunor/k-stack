#include <kstack/algorithms/TextNormalizer.hpp>

#include <algorithm>
#include <cctype>

namespace kstack::algorithms {

std::string normalizeText(std::string_view text) {
    const auto whitespace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!text.empty() && whitespace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && whitespace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return result;
}

} // namespace kstack::algorithms
