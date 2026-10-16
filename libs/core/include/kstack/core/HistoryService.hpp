/*
 * Author: Runor Ewhro
 * Description: owns visible history and coordinates optional durable storage.
 */
#pragma once

#include <kstack/core/ClipboardEntry.hpp>
#include <kstack/core/IHistoryStore.hpp>
#include <kstack/core/RetentionPolicy.hpp>
#include <codelang/CodeLanguage.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kstack::core {

class HistoryService {
public:
    explicit HistoryService(RetentionPolicy policy = RetentionPolicy{});
    void attachStore(IHistoryStore& store);
    [[nodiscard]] std::optional<ClipboardEntry> add(std::string text);
    [[nodiscard]] const std::vector<ClipboardEntry>& entries() const noexcept;
    [[nodiscard]] bool remove(std::uint64_t id);
    void clear();
    // settles close calls when naming the language of captured code
    void setCodeLanguageTieBreaker(codelang::CodeLanguageTieBreaker breakTie);

private:
    RetentionPolicy policy_;
    std::vector<ClipboardEntry> entries_;
    std::uint64_t nextId_ = 1;
    codelang::CodeLanguageTieBreaker breakTie_;
    IHistoryStore* store_ = nullptr;
};

} // namespace kstack::core
