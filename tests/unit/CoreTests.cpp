#include <kstack/algorithms/ContentHasher.hpp>
#include <kstack/algorithms/SearchRanker.hpp>
#include <kstack/core/HistoryService.hpp>
#include <kstack/core/SearchService.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
} // namespace

int main() {
    kstack::core::HistoryService history(kstack::core::RetentionPolicy{2});
    const auto beforeCapture = std::chrono::system_clock::now();
    require(history.add("First URL: https://example.com").has_value(), "first capture failed");
    const auto afterCapture = std::chrono::system_clock::now();
    require(history.entries().front().capturedAt >= beforeCapture &&
                history.entries().front().capturedAt <= afterCapture,
            "capture timestamp failed");
    require(!history.add("First URL: https://example.com").has_value(), "duplicate captured");
    require(history.add("Second text").has_value(), "second capture failed");
    require(history.add("Third text").has_value(), "third capture failed");
    require(history.entries().size() == 2, "retention limit failed");
    require(history.entries().front().text == "Third text", "newest item order failed");
    require(kstack::core::SearchService(history).search("SECOND").size() == 1,
            "case-insensitive search failed");
    require(kstack::core::SearchService(history).search("First").empty(), "evicted item found");
    require(kstack::algorithms::searchScore("Hello world", "hello") == 2,
            "prefix ranking failed");
    require(kstack::algorithms::contentHash("a") != kstack::algorithms::contentHash("b"),
            "content fingerprint failed");
}
