#pragma once

#include <kstack/core/ClipboardEntry.hpp>
#include <kstack/core/HistoryService.hpp>

#include <string_view>
#include <vector>

namespace kstack::core {

class SearchService {
public:
    explicit SearchService(const HistoryService& history) noexcept;
    [[nodiscard]] std::vector<ClipboardEntry> search(std::string_view query) const;

private:
    const HistoryService& history_;
};

} // namespace kstack::core
