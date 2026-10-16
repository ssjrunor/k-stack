#include <kstack/storage/EntryRepository.hpp>

#include <sqlite3.h>
#include <stdexcept>

namespace kstack::storage {

EntryRepository::EntryRepository(Database& database, EncryptionProvider& encryption) noexcept
    : database_(database), encryption_(encryption) {}

void EntryRepository::insertText(std::string_view text, std::int64_t capturedAtUnixMs) {
    const auto bytes = std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    const auto ciphertext = encryption_.encrypt(bytes);
    sqlite3_stmt* statement = nullptr;
    constexpr auto sql = "INSERT INTO entries (captured_at_ms, payload) VALUES (?, ?)";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
    const int timeResult = sqlite3_bind_int64(statement, 1, capturedAtUnixMs);
    const int blobResult = sqlite3_bind_blob64(statement, 2, ciphertext.data(), ciphertext.size(), SQLITE_TRANSIENT);
    const int stepResult = timeResult == SQLITE_OK && blobResult == SQLITE_OK
                               ? sqlite3_step(statement)
                               : SQLITE_ERROR;
    sqlite3_finalize(statement);
    if (stepResult != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
}

} // namespace kstack::storage
