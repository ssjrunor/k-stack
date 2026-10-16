#include <kstack/core/HistoryService.hpp>
#include <kstack/algorithms/DuplicateDetector.hpp>

#include <algorithm>
#include <chrono>
#include <utility>

namespace kstack::core {

HistoryService::HistoryService(RetentionPolicy policy) : policy_(policy) {}

std::optional<ClipboardEntry> HistoryService::add(std::string text) {
    if (text.empty() || policy_.maxEntries() == 0) {
        return std::nullopt;
    }
    if (!entries_.empty() && algorithms::isDuplicate(entries_.front().text, text)) {
        return std::nullopt;
    }
    ClipboardEntry entry{nextId_++, std::move(text), std::chrono::system_clock::now(), false};
    entries_.insert(entries_.begin(), entry);
    if (entries_.size() > policy_.maxEntries()) {
        entries_.resize(policy_.maxEntries());
    }
    return entry;
}

const std::vector<ClipboardEntry>& HistoryService::entries() const noexcept { return entries_; }

bool HistoryService::remove(std::uint64_t id) {
    const auto oldSize = entries_.size();
    std::erase_if(entries_, [id](const ClipboardEntry& entry) { return entry.id == id; });
    return entries_.size() != oldSize;
}

void HistoryService::clear() noexcept { entries_.clear(); }

} // namespace kstack::core
