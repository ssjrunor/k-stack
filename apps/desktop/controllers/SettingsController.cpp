/*
  Author: Runor Ewhro
  Description: starts and stops clipboard capture and reports when the
               platform monitor cannot read the clipboard.
*/
#include "SettingsController.hpp"

namespace kstack::desktop {

SettingsController::SettingsController(ClipboardController& clipboard, QObject* parent)
    : QObject(parent), clipboard_(clipboard) {}

bool SettingsController::captureEnabled() const noexcept { return captureEnabled_; }

QString SettingsController::captureStatus() const { return captureStatus_; }

bool SettingsController::captureBlocked() const noexcept { return captureBlocked_; }

// keep the exposed state tied to a successfully started monitor, so the menu
// cannot claim capture is running when pasteboard access is unavailable
void SettingsController::setCaptureEnabled(bool enabled) {
    if (captureEnabled_ == enabled &&
        (enabled || !captureBlocked_)) {
        return;
    }
    if (enabled) {
        captureEnabled_ = clipboard_.start();
        captureBlocked_ = !captureEnabled_;
        captureStatus_ = captureEnabled_ ? QStringLiteral("Capturing")
                                         : QStringLiteral("Clipboard capture unavailable");
    } else {
        clipboard_.stop();
        captureEnabled_ = false;
        captureBlocked_ = false;
        captureStatus_ = QStringLiteral("Paused");
    }
    emit captureEnabledChanged();
    emit captureStatusChanged();
}

} // namespace kstack::desktop
