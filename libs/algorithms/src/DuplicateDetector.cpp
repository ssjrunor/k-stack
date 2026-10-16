#include <kstack/algorithms/DuplicateDetector.hpp>

namespace kstack::algorithms {

bool isDuplicate(std::string_view left, std::string_view right) {
    return left == right;
}

} // namespace kstack::algorithms
