/*
 * Author: Runor Ewhro
 * Description: creates the look-and-feel and system-service objects for the
 *              running platform. Each platform provides its own definition
 *              of these functions, chosen by the build, so shared code never
 *              checks which system it runs on.
 */
#pragma once

#include "PlatformUi.hpp"
#include "SystemServices.hpp"

#include <memory>

namespace kstack::desktop {

std::unique_ptr<PlatformUi> createPlatformUi();
std::unique_ptr<SystemServices> createSystemServices();

} // namespace kstack::desktop
