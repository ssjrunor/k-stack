/*
  Author: Runor Ewhro
  Description: starts and stops clipboard capture and reports when the
               platform monitor cannot read the clipboard.
*/
#include "SettingsController.hpp"

namespace kstack::desktop {

SettingsController::SettingsController(ClipboardController& clipboard, QObject* parent)
    : QObject(parent), clipboard_(clipboard) {}

SettingsController::~SettingsController() { clipboard_.stop(); }

bool SettingsController::captureEnabled() const noexcept { return captureEnabled_; }

QString SettingsController::captureStatus() const { return captureStatus_; }

bool SettingsController::captureBlocked() const noexcept { return captureBlocked_; }

bool SettingsController::persistentHistory() const noexcept {
#ifdef KSTACK_HAS_PERSISTENCE
    return true;
#else
    return false;
#endif
}

void SettingsController::storageFailed(const QString& message) {
    Q_UNUSED(message)
    clipboard_.stop();
    captureEnabled_ = false;
    captureBlocked_ = true;
    captureStatus_ = QStringLiteral("History save failed");
    emit captureEnabledChanged();
    emit captureStatusChanged();
}

// keep the exposed state tied to a successfully started monitor, so the menu
// cannot claim capture is running when pasteboard access is unavailable
void SettingsController::setCaptureEnabled(bool enabled) {
    if (captureEnabled_ == enabled &&
        (enabled || !captureBlocked_)) {
        return;
    }
    if (enabled) {
        captureEnabled_ = clipboard_.start([this](bool allowed) { accessChanged(allowed); });
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

void SettingsController::accessChanged(bool allowed) {
    if (captureEnabled_ == allowed && captureBlocked_ == !allowed) {
        return;
    }
    captureEnabled_ = allowed;
    captureBlocked_ = !allowed;
    captureStatus_ = allowed ? QStringLiteral("Capturing")
                             : QStringLiteral("Clipboard capture unavailable");
    emit captureEnabledChanged();
    emit captureStatusChanged();
}

} // namespace kstack::desktop
