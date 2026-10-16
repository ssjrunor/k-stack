/*
 * Author: Runor Ewhro
 * Description: supplies the portable look-and-feel and system services on
 *              platforms that have no dedicated implementation yet; a new
 *              platform replaces this file with its own factory.
 */
#include "../PlatformFactory.hpp"

namespace kstack::desktop {

std::unique_ptr<PlatformUi> createPlatformUi() { return std::make_unique<PlatformUi>(); }

std::unique_ptr<SystemServices> createSystemServices() { return std::make_unique<SystemServices>(); }

} // namespace kstack::desktop
