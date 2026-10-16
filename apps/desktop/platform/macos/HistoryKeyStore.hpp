/*
 * Author: Runor Ewhro
 * Description: retrieves the history database key from the macOS Keychain.
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace kstack::desktop {

[[nodiscard]] std::array<std::uint8_t, 32> historyDatabaseKey(
    bool databaseExists, const std::string& service = "org.runorewhro.clipboard-history");
#ifdef KSTACK_ENABLE_STRESS_PROFILE
void deleteStressHistoryKey(const std::string& service);
#endif

} // namespace kstack::desktop
