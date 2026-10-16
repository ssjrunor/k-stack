/*
 * Author: Runor Ewhro
 * Description: records and retrieves clipboard history inside SQLCipher,
 *              reusing ids for exact duplicates without pruning old entries.
 */
#include <kstack/storage/EntryRepository.hpp>

#include <kstack/algorithms/ContentHasher.hpp>

#include <sqlite3.h>

#include <array>
#include <chrono>
#include <stdexcept>
#include <string_view>

namespace kstack::storage {
namespace {
class Statement {
public:
    Statement(sqlite3* database, const char* sql) : database_(database) {
        if (sqlite3_prepare_v2(database, sql, -1, &handle_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
    }
    ~Statement() { sqlite3_finalize(handle_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    [[nodiscard]] sqlite3_stmt* get() const noexcept { return handle_; }
    void check(int result) const {
        if (result != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database_));
        }
    }
    void done() const {
        if (sqlite3_step(handle_) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database_));
        }
    }

private:
    sqlite3* database_;
    sqlite3_stmt* handle_ = nullptr;
};

void execute(sqlite3* database, const char* sql) {
    if (sqlite3_exec(database, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

std::array<std::uint8_t, 8> hashBytes(std::string_view text) {
    const std::uint64_t hash = algorithms::contentHash(text);
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(hash >> (i * 8));
    }
    return bytes;
}

std::int64_t milliseconds(std::chrono::system_clock::time_point time) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}
} // namespace

EntryRepository::EntryRepository(Database& database) noexcept : database_(database) {}

std::vector<core::ClipboardEntry> EntryRepository::loadAll() {
    Statement query(database_.handle(),
                    "SELECT id, captured_at_ms, content, pinned, kind, code_language, "
                    "classification_version FROM entries "
                    "ORDER BY captured_at_ms DESC, id DESC");
    std::vector<core::ClipboardEntry> entries;
    int result = SQLITE_OK;
    while ((result = sqlite3_step(query.get())) == SQLITE_ROW) {
        const auto* content = reinterpret_cast<const char*>(sqlite3_column_text(query.get(), 2));
        const int length = sqlite3_column_bytes(query.get(), 2);
        if (!content || length < 0) {
            throw std::runtime_error("History database contains invalid text");
        }
        core::ClipboardEntry entry;
        entry.id = static_cast<std::uint64_t>(sqlite3_column_int64(query.get(), 0));
        entry.capturedAt = std::chrono::system_clock::time_point{
            std::chrono::milliseconds(sqlite3_column_int64(query.get(), 1))};
        entry.text.assign(content, static_cast<std::size_t>(length));
        entry.pinned = sqlite3_column_int(query.get(), 3) != 0;
        const int kind = sqlite3_column_int(query.get(), 4);
        const auto* language = reinterpret_cast<const char*>(sqlite3_column_text(query.get(), 5));
        const int languageLength = sqlite3_column_bytes(query.get(), 5);
        const bool validKind = kind >= 0 && kind <= static_cast<int>(core::ContentKind::Secret);
        entry.metadataReady = validKind && language && languageLength >= 0 &&
                              sqlite3_column_int(query.get(), 6) == core::kEntryClassificationVersion;
        if (entry.metadataReady) {
            entry.kind = static_cast<core::ContentKind>(kind);
            entry.codeLanguage.assign(language, static_cast<std::size_t>(languageLength));
        }
        entries.push_back(std::move(entry));
    }
    if (result != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
    return entries;
}

std::uint64_t EntryRepository::record(const core::ClipboardEntry& entry) {
    sqlite3* db = database_.handle();
    const auto hash = hashBytes(entry.text);
    execute(db, "BEGIN IMMEDIATE");
    try {
        std::uint64_t existingId = 0;
        {
            Statement find(db, "SELECT id, content FROM entries WHERE content_hash = ?");
            find.check(sqlite3_bind_blob(find.get(), 1, hash.data(), static_cast<int>(hash.size()), SQLITE_TRANSIENT));
            int result = SQLITE_OK;
            while ((result = sqlite3_step(find.get())) == SQLITE_ROW) {
                const auto* content = reinterpret_cast<const char*>(sqlite3_column_text(find.get(), 1));
                const int length = sqlite3_column_bytes(find.get(), 1);
                if (content && std::string_view(content, static_cast<std::size_t>(length)) == entry.text) {
                    existingId = static_cast<std::uint64_t>(sqlite3_column_int64(find.get(), 0));
                    break;
                }
            }
            if (result != SQLITE_ROW && result != SQLITE_DONE) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
        }
        if (existingId != 0) {
            Statement update(db, "UPDATE entries SET captured_at_ms = ?, kind = ?, "
                                 "code_language = ?, classification_version = ? WHERE id = ?");
            update.check(sqlite3_bind_int64(update.get(), 1, milliseconds(entry.capturedAt)));
            update.check(sqlite3_bind_int(update.get(), 2, static_cast<int>(entry.kind)));
            update.check(sqlite3_bind_text64(update.get(), 3, entry.codeLanguage.data(),
                                             entry.codeLanguage.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
            update.check(sqlite3_bind_int(update.get(), 4, core::kEntryClassificationVersion));
            update.check(sqlite3_bind_int64(update.get(), 5, static_cast<sqlite3_int64>(existingId)));
            update.done();
            execute(db, "COMMIT");
            return existingId;
        }
        Statement insert(db, "INSERT INTO entries (captured_at_ms, content_hash, content, pinned, "
                             "kind, code_language, classification_version) VALUES (?, ?, ?, ?, ?, ?, ?)");
        insert.check(sqlite3_bind_int64(insert.get(), 1, milliseconds(entry.capturedAt)));
        insert.check(sqlite3_bind_blob(insert.get(), 2, hash.data(), static_cast<int>(hash.size()), SQLITE_TRANSIENT));
        insert.check(sqlite3_bind_text64(insert.get(), 3, entry.text.data(), entry.text.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
        insert.check(sqlite3_bind_int(insert.get(), 4, entry.pinned ? 1 : 0));
        insert.check(sqlite3_bind_int(insert.get(), 5, static_cast<int>(entry.kind)));
        insert.check(sqlite3_bind_text64(insert.get(), 6, entry.codeLanguage.data(),
                                         entry.codeLanguage.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
        insert.check(sqlite3_bind_int(insert.get(), 7, core::kEntryClassificationVersion));
        insert.done();
        const auto id = static_cast<std::uint64_t>(sqlite3_last_insert_rowid(db));
        execute(db, "COMMIT");
        return id;
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}

void EntryRepository::updateMetadata(std::span<const core::ClipboardEntry> entries) {
    if (entries.empty()) return;
    sqlite3* db = database_.handle();
    execute(db, "BEGIN IMMEDIATE");
    try {
        for (const auto& entry : entries) {
            Statement update(db, "UPDATE entries SET kind = ?, code_language = ?, "
                                 "classification_version = ? WHERE id = ?");
            update.check(sqlite3_bind_int(update.get(), 1, static_cast<int>(entry.kind)));
            update.check(sqlite3_bind_text64(update.get(), 2, entry.codeLanguage.data(),
                                             entry.codeLanguage.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
            update.check(sqlite3_bind_int(update.get(), 3, core::kEntryClassificationVersion));
            update.check(sqlite3_bind_int64(update.get(), 4, static_cast<sqlite3_int64>(entry.id)));
            update.done();
        }
        execute(db, "COMMIT");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}

void EntryRepository::remove(std::uint64_t id) {
    Statement statement(database_.handle(), "DELETE FROM entries WHERE id = ?");
    statement.check(sqlite3_bind_int64(statement.get(), 1, static_cast<sqlite3_int64>(id)));
    statement.done();
}

void EntryRepository::clear() { execute(database_.handle(), "DELETE FROM entries"); }
} // namespace kstack::storage
