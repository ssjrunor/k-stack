/*
 * Author: Runor Ewhro
 * Description: owns a verified SQLCipher connection and versioned migrations.
 */
#pragma once

#include <cstdint>
#include <span>
#include <string>

struct sqlite3;

namespace kstack::storage {

// Opens and verifies a keyed SQLCipher database before applying migrations.
class Database {
public:
    Database(const std::string& path, std::span<const std::uint8_t> key);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    [[nodiscard]] sqlite3* handle() const noexcept;

private:
    sqlite3* handle_ = nullptr;
};

} // namespace kstack::storage
