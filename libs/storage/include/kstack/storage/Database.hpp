#pragma once

#include <string>

struct sqlite3;

namespace kstack::storage {

// Opens an SQLite connection. Migrations are intentionally not automatic yet.
class Database {
public:
    explicit Database(const std::string& path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    [[nodiscard]] sqlite3* handle() const noexcept;

private:
    sqlite3* handle_ = nullptr;
};

} // namespace kstack::storage
