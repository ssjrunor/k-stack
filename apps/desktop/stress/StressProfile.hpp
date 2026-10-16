/*
 * Author: Runor Ewhro
 * Description: validates isolated stress profiles and verifies generated
 *              entries in the normal encrypted history.
 */
#pragma once

#include <QString>

#include <string>

namespace kstack::desktop {

struct StressProfile {
    QString directory;
    std::string keyService;
};

[[nodiscard]] StressProfile validatedStressProfile(const QString& directory);
void reportStressProfile(const StressProfile& profile);
void removeStressProfileKey(const StressProfile& profile);
void verifyLiveStressEntries(const QString& manifestPath);

} // namespace kstack::desktop
