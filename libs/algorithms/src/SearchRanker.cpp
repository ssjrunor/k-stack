#include <kstack/algorithms/SearchRanker.hpp>
#include <kstack/algorithms/TextNormalizer.hpp>

namespace kstack::algorithms {

int searchScore(std::string_view content, std::string_view query) {
    const auto normalizedQuery = normalizeText(query);
    if (normalizedQuery.empty()) {
        return 1;
    }
    const auto normalizedContent = normalizeText(content);
    const auto position = normalizedContent.find(normalizedQuery);
    if (position == std::string::npos) {
        return 0;
    }
    if (normalizedContent == normalizedQuery) {
        return 3;
    }
    return position == 0 ? 2 : 1;
}

} // namespace kstack::algorithms
