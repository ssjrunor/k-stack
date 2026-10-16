/*
 * Author: Runor Ewhro
 * Description: defines the commit-before-display contract for history storage.
 */
#pragma once

#include <kstack/core/ClipboardEntry.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace kstack::core {

// Storage implementations must commit each operation before returning.
class IHistoryStore {
public:
    virtual ~IHistoryStore() = default;
    [[nodiscard]] virtual std::vector<ClipboardEntry> loadAll() = 0;
    // Repeated content may reuse an existing id and update its capture time.
    [[nodiscard]] virtual std::uint64_t record(const ClipboardEntry& entry) = 0;
    virtual void updateMetadata(std::span<const ClipboardEntry> entries) = 0;
    virtual void remove(std::uint64_t id) = 0;
    virtual void clear() = 0;
};

} // namespace kstack::core
