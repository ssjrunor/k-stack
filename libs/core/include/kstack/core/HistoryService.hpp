#pragma once

#include <kstack/core/ClipboardEntry.hpp>
#include <kstack/core/RetentionPolicy.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kstack::core {

class HistoryService {
public:
    explicit HistoryService(RetentionPolicy policy = RetentionPolicy{});
    [[nodiscard]] std::optional<ClipboardEntry> add(std::string text);
    [[nodiscard]] const std::vector<ClipboardEntry>& entries() const noexcept;
    [[nodiscard]] bool remove(std::uint64_t id);
    void clear() noexcept;

private:
    RetentionPolicy policy_;
    std::vector<ClipboardEntry> entries_;
    std::uint64_t nextId_ = 1;
};

} // namespace kstack::core
