/*
 * Author: Runor Ewhro
 * Description: exposes durable clipboard entries through the core store interface.
 */
#pragma once

#include <kstack/storage/Database.hpp>
#include <kstack/core/IHistoryStore.hpp>

#include <cstdint>
#include <vector>

namespace kstack::storage {

// SQLCipher encrypts the entire database, including hash indexes and text.
class EntryRepository final : public core::IHistoryStore {
public:
    explicit EntryRepository(Database& database) noexcept;
    [[nodiscard]] std::vector<core::ClipboardEntry> loadAll() override;
    [[nodiscard]] std::uint64_t record(const core::ClipboardEntry& entry) override;
    void updateMetadata(std::span<const core::ClipboardEntry> entries) override;
    void remove(std::uint64_t id) override;
    void clear() override;

private:
    Database& database_;
};

} // namespace kstack::storage
