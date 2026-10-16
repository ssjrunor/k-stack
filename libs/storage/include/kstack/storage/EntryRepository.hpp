#pragma once

#include <kstack/storage/Database.hpp>
#include <kstack/storage/EncryptionProvider.hpp>

#include <cstdint>
#include <string_view>

namespace kstack::storage {

// The repository only accepts encrypted payloads through its provider.
class EntryRepository {
public:
    EntryRepository(Database& database, EncryptionProvider& encryption) noexcept;
    void insertText(std::string_view text, std::int64_t capturedAtUnixMs);

private:
    Database& database_;
    EncryptionProvider& encryption_;
};

} // namespace kstack::storage
