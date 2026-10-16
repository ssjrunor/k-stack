#pragma once

#include <kstack/storage/Database.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace kstack::storage {

class SettingsRepository {
public:
    explicit SettingsRepository(Database& database) noexcept;
    [[nodiscard]] std::optional<std::string> get(std::string_view key) const;
    void set(std::string_view key, std::string_view value);

private:
    Database& database_;
};

} // namespace kstack::storage
