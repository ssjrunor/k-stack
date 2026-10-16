/*
 * Author: Runor Ewhro
 * Description: searches history once per entry while preserving exact,
 *              prefix, then substring ranking and capture order.
 */
#include <kstack/core/SearchService.hpp>
#include <kstack/algorithms/TextNormalizer.hpp>

#include <iterator>

namespace kstack::core {

SearchService::SearchService(const HistoryService& history) noexcept : history_(history) {}

std::vector<ClipboardEntry> SearchService::search(std::string_view query) const {
    const auto& entries = history_.entries();
    const std::string needle = algorithms::normalizeText(query);
    if (needle.empty()) {
        return entries;
    }

    std::vector<ClipboardEntry> exact;
    std::vector<ClipboardEntry> prefix;
    std::vector<ClipboardEntry> substring;
    for (const auto& entry : entries) {
        const std::string content = algorithms::normalizeText(entry.text);
        const auto position = content.find(needle);
        if (position == std::string::npos) {
            continue;
        }
        if (content == needle) {
            exact.push_back(entry);
        } else if (position == 0) {
            prefix.push_back(entry);
        } else {
            substring.push_back(entry);
        }
    }
    std::vector<ClipboardEntry> result;
    result.reserve(exact.size() + prefix.size() + substring.size());
    result.insert(result.end(), std::make_move_iterator(exact.begin()),
                  std::make_move_iterator(exact.end()));
    result.insert(result.end(), std::make_move_iterator(prefix.begin()),
                  std::make_move_iterator(prefix.end()));
    result.insert(result.end(), std::make_move_iterator(substring.begin()),
                  std::make_move_iterator(substring.end()));
    return result;
}

} // namespace kstack::core
