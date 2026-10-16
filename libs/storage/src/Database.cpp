/*
 * Author: Runor Ewhro
 * Description: checks the database key before reading or migrating history.
 */
#include <kstack/storage/Database.hpp>

#include "MigrationSql.hpp"

#include <sqlite3.h>
#include <stdexcept>

namespace kstack::storage {

namespace {
void execute(sqlite3* handle, const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(handle, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(handle);
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

int schemaVersion(sqlite3* handle) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(handle, "PRAGMA user_version", -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(handle));
    }
    const int result = sqlite3_step(statement);
    const int version = result == SQLITE_ROW ? sqlite3_column_int(statement, 0) : -1;
    sqlite3_finalize(statement);
    if (result != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(handle));
    }
    return version;
}

void migrate(sqlite3* handle, const char* migration, int version) {
    execute(handle, "BEGIN IMMEDIATE");
    try {
        execute(handle, migration);
        execute(handle, ("PRAGMA user_version = " + std::to_string(version)).c_str());
        execute(handle, "COMMIT");
    } catch (...) {
        sqlite3_exec(handle, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}
} // namespace

Database::Database(const std::string& path, std::span<const std::uint8_t> key) {
    if (key.size() != 32) {
        throw std::invalid_argument("History database key must be 32 bytes");
    }
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX |
                      SQLITE_OPEN_NOFOLLOW;
    if (sqlite3_open_v2(path.c_str(), &handle_, flags, nullptr) != SQLITE_OK) {
        const std::string message = handle_ ? sqlite3_errmsg(handle_) : "SQLite could not allocate a connection";
        sqlite3_close(handle_);
        handle_ = nullptr;
        throw std::runtime_error(message);
    }
    try {
        if (sqlite3_key(handle_, key.data(), static_cast<int>(key.size())) != SQLITE_OK) {
            throw std::runtime_error("Could not key history database");
        }
        // SQLCipher defers key checking until the first page is read.
        execute(handle_, "SELECT count(*) FROM sqlite_master");
        execute(handle_, "PRAGMA temp_store = MEMORY");
        sqlite3_busy_timeout(handle_, 5000);
        const int version = schemaVersion(handle_);
        if (version > 2) {
            throw std::runtime_error("History database was created by a newer version");
        }
        if (version == 0) {
            migrate(handle_, kInitialMigration, 1);
        }
        if (version <= 1) {
            migrate(handle_, kEntryMetadataMigration, 2);
        }
        execute(handle_, "PRAGMA journal_mode = WAL");
    } catch (...) {
        sqlite3_close(handle_);
        handle_ = nullptr;
        throw;
    }
}

Database::~Database() { sqlite3_close(handle_); }

sqlite3* Database::handle() const noexcept { return handle_; }

} // namespace kstack::storage
