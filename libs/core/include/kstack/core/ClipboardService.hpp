#pragma once

#include <kstack/core/HistoryService.hpp>

#include <string_view>

namespace kstack::core {

class ClipboardService {
public:
    explicit ClipboardService(HistoryService& history) noexcept;
    [[nodiscard]] bool captureText(std::string_view text);

private:
    HistoryService& history_;
};

} // namespace kstack::core
