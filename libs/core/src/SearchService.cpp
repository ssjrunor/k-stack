#include <kstack/core/SearchService.hpp>
#include <kstack/algorithms/SearchRanker.hpp>

#include <algorithm>

namespace kstack::core {

SearchService::SearchService(const HistoryService& history) noexcept : history_(history) {}

std::vector<ClipboardEntry> SearchService::search(std::string_view query) const {
    std::vector<ClipboardEntry> result;
    for (const auto& entry : history_.entries()) {
        if (algorithms::searchScore(entry.text, query) > 0) {
            result.push_back(entry);
        }
    }
    std::stable_sort(result.begin(), result.end(), [query](const auto& left, const auto& right) {
        return algorithms::searchScore(left.text, query) >
               algorithms::searchScore(right.text, query);
    });
    return result;
}

} // namespace kstack::core
