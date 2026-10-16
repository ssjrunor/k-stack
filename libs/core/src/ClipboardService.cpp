#include <kstack/core/ClipboardService.hpp>

namespace kstack::core {

ClipboardService::ClipboardService(HistoryService& history) noexcept : history_(history) {}

bool ClipboardService::captureText(std::string_view text) {
    return history_.add(std::string(text)).has_value();
}

} // namespace kstack::core
