#include <kstack/algorithms/ContentHasher.hpp>

namespace kstack::algorithms {

std::uint64_t contentHash(std::string_view content) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : content) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace kstack::algorithms
