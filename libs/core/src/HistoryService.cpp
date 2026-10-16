/*
 * Author: Runor Ewhro
 * Description: commits captures and removals before changing visible history.
 */
#include <kstack/core/HistoryService.hpp>
#include <codelang/CodeLanguage.hpp>
#include <kstack/algorithms/DuplicateDetector.hpp>

#include <algorithm>
#include <chrono>
#include <utility>

namespace kstack::core {

HistoryService::HistoryService(RetentionPolicy policy) : policy_(policy) {}

void HistoryService::attachStore(IHistoryStore& store) {
    auto loaded = store.loadAll();
    std::vector<ClipboardEntry> reclassified;
    for (auto& entry : loaded) {
        if (!entry.metadataReady) {
            entry.kind = classifyContent(entry.text);
            entry.codeLanguage.clear();
            if (entry.kind == ContentKind::Code) {
                if (const auto guess = codelang::detectCodeLanguage(entry.text, breakTie_)) {
                    entry.codeLanguage = guess->language->name;
                }
            }
            entry.metadataReady = true;
            reclassified.push_back(entry);
        }
    }
    if (!reclassified.empty()) {
        store.updateMetadata(reclassified);
    }
    entries_ = std::move(loaded);
    store_ = &store;
}

std::optional<ClipboardEntry> HistoryService::add(std::string text) {
    if (text.empty() || (!store_ && policy_.maxEntries() == 0)) {
        return std::nullopt;
    }
    if (!entries_.empty() && algorithms::isDuplicate(entries_.front().text, text)) {
        return std::nullopt;
    }
    const ContentKind kind = classifyContent(text);
    std::string codeLanguage;
    if (kind == ContentKind::Code) {
        if (const auto guess = codelang::detectCodeLanguage(text, breakTie_)) {
            codeLanguage = guess->language->name;
        }
    }
    ClipboardEntry entry{0, std::move(text), std::chrono::system_clock::now(), false, kind,
                         std::move(codeLanguage)};
    if (store_) {
        entry.id = store_->record(entry);
        std::erase_if(entries_, [&entry](const ClipboardEntry& existing) {
            return existing.id == entry.id;
        });
    } else {
        entry.id = nextId_++;
    }
    entries_.insert(entries_.begin(), entry);
    if (!store_ && entries_.size() > policy_.maxEntries()) {
        entries_.resize(policy_.maxEntries());
    }
    return entry;
}

const std::vector<ClipboardEntry>& HistoryService::entries() const noexcept { return entries_; }

bool HistoryService::remove(std::uint64_t id) {
    const auto oldSize = entries_.size();
    if (std::none_of(entries_.begin(), entries_.end(), [id](const ClipboardEntry& entry) {
            return entry.id == id;
        })) {
        return false;
    }
    if (store_) {
        store_->remove(id);
    }
    std::erase_if(entries_, [id](const ClipboardEntry& entry) { return entry.id == id; });
    return entries_.size() != oldSize;
}

void HistoryService::clear() {
    if (store_) {
        store_->clear();
    }
    entries_.clear();
}

void HistoryService::setCodeLanguageTieBreaker(codelang::CodeLanguageTieBreaker breakTie) {
    breakTie_ = std::move(breakTie);
}

} // namespace kstack::core
