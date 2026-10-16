#include <kstack/storage/Database.hpp>

#include <sqlite3.h>
#include <stdexcept>

namespace kstack::storage {

Database::Database(const std::string& path) {
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(path.c_str(), &handle_, flags, nullptr) != SQLITE_OK) {
        const std::string message = handle_ ? sqlite3_errmsg(handle_) : "SQLite could not allocate a connection";
        sqlite3_close(handle_);
        handle_ = nullptr;
        throw std::runtime_error(message);
    }
    sqlite3_busy_timeout(handle_, 5000);
}

Database::~Database() { sqlite3_close(handle_); }

sqlite3* Database::handle() const noexcept { return handle_; }

} // namespace kstack::storage
