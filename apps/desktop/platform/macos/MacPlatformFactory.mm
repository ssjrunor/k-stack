/*
 * Author: Runor Ewhro
 * Description: supplies the macOS look-and-feel and system services.
 */
#include "../PlatformFactory.hpp"
#include "MacPlatformUi.hpp"
#include "MacSystemServices.hpp"

namespace kstack::desktop {

std::unique_ptr<PlatformUi> createPlatformUi() { return std::make_unique<MacPlatformUi>(); }

std::unique_ptr<SystemServices> createSystemServices() { return std::make_unique<MacSystemServices>(); }

} // namespace kstack::desktop
