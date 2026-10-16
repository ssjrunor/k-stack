#include <kstack/core/CapturePipeline.hpp>

namespace kstack::core {

CapturePipeline::CapturePipeline(ClipboardService& clipboard) noexcept : clipboard_(clipboard) {}

bool CapturePipeline::processText(std::string_view text) {
    return clipboard_.captureText(text);
}

} // namespace kstack::core
