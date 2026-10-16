/*
 * Author: Runor Ewhro
 * Description: verifies encrypted database creation, restart, deduplication,
 *              deletion, and refusal to open with the wrong key.
 */
#include <kstack/storage/Database.hpp>
#include <kstack/storage/EntryRepository.hpp>
#include <kstack/core/HistoryService.hpp>
#include <kstack/core/SearchService.hpp>

#include <sqlite3.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

double timedCount(sqlite3* database, const char* sql) {
    sqlite3_stmt* statement = nullptr;
    require(sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) == SQLITE_OK,
            "could not prepare search comparison");
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 200; ++i) {
        require(sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_int(statement, 0) == 1,
                "search comparison returned the wrong count");
        require(sqlite3_reset(statement) == SQLITE_OK, "could not reset search comparison");
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    sqlite3_finalize(statement);
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

struct TestDirectory {
    std::filesystem::path path;
    TestDirectory() {
        const auto base = std::filesystem::canonical(std::filesystem::temp_directory_path());
        const std::string pattern = (base / "kstack-history-XXXXXX").string();
        std::vector<char> mutablePattern(pattern.begin(), pattern.end());
        mutablePattern.push_back('\0');
        char* created = mkdtemp(mutablePattern.data());
        if (!created) throw std::runtime_error("Cannot create test directory");
        path = created;
    }
    ~TestDirectory() { std::filesystem::remove_all(path); }
};

void checkEncryptedHistory() {
    TestDirectory directory;
    const auto path = (directory.path / "history.db").string();
    std::array<std::uint8_t, 32> key{};
    key.fill(0x42); // synthetic test key, never used for real history
    std::uint64_t originalId = 0;
    {
        kstack::storage::Database database(path, key);
        kstack::storage::EntryRepository entries(database);
        for (int i = 0; i < 5000; ++i) {
            kstack::core::ClipboardEntry entry;
            entry.text = "synthetic private content " + std::to_string(i);
            entry.capturedAt = std::chrono::system_clock::time_point{std::chrono::milliseconds(i)};
            const auto id = entries.record(entry);
            if (i == 17) originalId = id;
        }
        require(entries.loadAll().size() == 5000, "history was pruned");
        // FTS5 is only a local experiment. Token search cannot replace the
        // app's arbitrary substring search without changing its results.
        require(sqlite3_exec(database.handle(),
                    "CREATE VIRTUAL TABLE entries_fts USING fts5(content, content='entries', content_rowid='id');"
                    "INSERT INTO entries_fts(entries_fts) VALUES('rebuild');",
                    nullptr, nullptr, nullptr) == SQLITE_OK, "FTS5 comparison setup failed");
        const double substringMs = timedCount(database.handle(),
            "SELECT count(*) FROM entries WHERE content LIKE '%4999%'");
        const double indexedMs = timedCount(database.handle(),
            "SELECT count(*) FROM entries_fts WHERE entries_fts MATCH '4999'");
        std::cout << "5,000 rows, 200 searches: substring " << substringMs
                  << " ms, FTS5 " << indexedMs << " ms\n";
    }
    {
        std::ifstream file(path, std::ios::binary);
        const std::string bytes(std::istreambuf_iterator<char>(file), {});
        require(bytes.find("synthetic private content") == std::string::npos,
                "plaintext entered the database file");
    }
    sqlite3* noKey = nullptr;
    require(sqlite3_open_v2(path.c_str(), &noKey, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK,
            "could not open encrypted file for negative check");
    require(sqlite3_exec(noKey, "SELECT count(*) FROM entries", nullptr, nullptr, nullptr) != SQLITE_OK,
            "encrypted database was readable without a key");
    sqlite3_close(noKey);
    auto wrongKey = key;
    wrongKey[0] ^= 1;
    bool wrongKeyRejected = false;
    try {
        kstack::storage::Database database(path, wrongKey);
    } catch (const std::runtime_error&) {
        wrongKeyRejected = true;
    }
    require(wrongKeyRejected, "wrong key opened the history");
    {
        kstack::storage::Database database(path, key);
        kstack::storage::EntryRepository entries(database);
        kstack::core::HistoryService history;
        const auto databaseLoadStart = std::chrono::steady_clock::now();
        const auto databaseRows = entries.loadAll();
        const auto databaseLoadMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - databaseLoadStart).count();
        require(databaseRows.size() == 5000, "encrypted database did not load all rows");
        const auto loadStart = std::chrono::steady_clock::now();
        history.attachStore(entries);
        const auto loadMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - loadStart).count();
        require(history.entries().size() == 5000, "core history did not survive restart");
        kstack::core::SearchService search(history);
        const auto searchStart = std::chrono::steady_clock::now();
        const auto matches = search.search("synthetic private content 17");
        const auto searchMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - searchStart).count();
        require(!matches.empty() && matches.front().id == originalId &&
                    matches.front().text == "synthetic private content 17",
                "core search could not retrieve an older saved item");
        std::cout << "5,000-row encrypted read: " << databaseLoadMs
                  << " ms, core restart load: " << loadMs
                  << " ms, substring search: " << searchMs << " ms\n";
        auto loaded = entries.loadAll();
        require(loaded.size() == 5000 && loaded.front().text == "synthetic private content 4999",
                "history did not survive restart");
        kstack::core::ClipboardEntry repeat;
        repeat.text = "synthetic private content 17";
        repeat.capturedAt = std::chrono::system_clock::time_point{std::chrono::milliseconds(6000)};
        require(entries.record(repeat) == originalId, "repeat did not reuse its id");
        loaded = entries.loadAll();
        require(loaded.size() == 5000 && loaded.front().id == originalId,
                "repeat created a second row or failed to move forward");
        entries.remove(originalId);
        require(entries.loadAll().size() == 4999, "delete did not persist");
        entries.clear();
        require(entries.loadAll().empty(), "clear did not persist");
    }
    {
        kstack::storage::Database database(path, key);
        kstack::storage::EntryRepository entries(database);
        require(entries.loadAll().empty(), "clear did not survive restart");
    }
}

void checkVersionOneUpgrade() {
    TestDirectory directory;
    const auto path = (directory.path / "legacy.db").string();
    std::array<std::uint8_t, 32> key{};
    key.fill(0x31);
    sqlite3* handle = nullptr;
    require(sqlite3_open_v2(path.c_str(), &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                            nullptr) == SQLITE_OK, "could not create version-one database");
    require(sqlite3_key(handle, key.data(), static_cast<int>(key.size())) == SQLITE_OK,
            "could not key version-one database");
    require(sqlite3_exec(handle,
        "CREATE TABLE entries (id INTEGER PRIMARY KEY, captured_at_ms INTEGER NOT NULL, "
        "content_hash BLOB NOT NULL, content TEXT NOT NULL, pinned INTEGER NOT NULL DEFAULT 0);"
        "INSERT INTO entries (captured_at_ms, content_hash, content) VALUES "
        "(1234, X'0102030405060708', 'https://example.com/legacy');"
        "PRAGMA user_version = 1;", nullptr, nullptr, nullptr) == SQLITE_OK,
        "could not populate version-one database");
    require(sqlite3_close(handle) == SQLITE_OK, "could not close version-one database");

    {
        kstack::storage::Database database(path, key);
        kstack::storage::EntryRepository entries(database);
        kstack::core::HistoryService history;
        history.attachStore(entries);
        require(history.entries().size() == 1 &&
                    history.entries().front().text == "https://example.com/legacy" &&
                    history.entries().front().kind == kstack::core::ContentKind::Link,
                "version-one history was not classified and preserved");
        kstack::core::ClipboardEntry code;
        code.text = "print('saved language')";
        code.kind = kstack::core::ContentKind::Code;
        code.codeLanguage = "Python";
        code.capturedAt = std::chrono::system_clock::time_point{std::chrono::milliseconds(2000)};
        (void)entries.record(code);
    }
    {
        kstack::storage::Database database(path, key);
        kstack::storage::EntryRepository entries(database);
        const auto loaded = entries.loadAll();
        require(loaded.size() == 2 && loaded[0].metadataReady &&
                    loaded[0].kind == kstack::core::ContentKind::Code &&
                    loaded[0].codeLanguage == "Python" &&
                    loaded[1].metadataReady &&
                    loaded[1].kind == kstack::core::ContentKind::Link,
                "classification metadata did not survive migration and restart");
    }
}
} // namespace

int main() {
    try {
        checkEncryptedHistory();
        checkVersionOneUpgrade();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
