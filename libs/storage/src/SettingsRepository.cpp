#include <kstack/storage/SettingsRepository.hpp>

#include <sqlite3.h>
#include <stdexcept>

namespace kstack::storage {

SettingsRepository::SettingsRepository(Database& database) noexcept : database_(database) {}

std::optional<std::string> SettingsRepository::get(std::string_view key) const {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database_.handle(), "SELECT value FROM settings WHERE key = ?", -1,
                           &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
    sqlite3_bind_text64(statement, 1, key.data(), key.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
    std::optional<std::string> result;
    const int stepResult = sqlite3_step(statement);
    if (stepResult == SQLITE_ROW) {
        const auto* value = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
        result = value ? std::string(value) : std::string{};
    }
    sqlite3_finalize(statement);
    if (stepResult != SQLITE_ROW && stepResult != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
    return result;
}

void SettingsRepository::set(std::string_view key, std::string_view value) {
    sqlite3_stmt* statement = nullptr;
    constexpr auto sql = "INSERT INTO settings (key, value) VALUES (?, ?) "
                         "ON CONFLICT(key) DO UPDATE SET value = excluded.value";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
    sqlite3_bind_text64(statement, 1, key.data(), key.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
    sqlite3_bind_text64(statement, 2, value.data(), value.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
    const int stepResult = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (stepResult != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
}

} // namespace kstack::storage
