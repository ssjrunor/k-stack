#pragma once

#include <kstack/core/ClipboardService.hpp>

#include <string_view>

namespace kstack::core {

class CapturePipeline {
public:
    explicit CapturePipeline(ClipboardService& clipboard) noexcept;
    [[nodiscard]] bool processText(std::string_view text);

private:
    ClipboardService& clipboard_;
};

} // namespace kstack::core
