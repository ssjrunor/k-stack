/*
  Author: Runor Ewhro
  Description: exposes capture state and monitor availability to the QML
               interface and the background tray control.
*/
#pragma once

#include "ClipboardController.hpp"

#include <QObject>
#include <QString>

namespace kstack::desktop {

class SettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool captureEnabled READ captureEnabled WRITE setCaptureEnabled NOTIFY captureEnabledChanged)
    Q_PROPERTY(QString captureStatus READ captureStatus NOTIFY captureStatusChanged)
    Q_PROPERTY(bool captureBlocked READ captureBlocked NOTIFY captureStatusChanged)
    Q_PROPERTY(bool persistentHistory READ persistentHistory CONSTANT)

public:
    explicit SettingsController(ClipboardController& clipboard, QObject* parent = nullptr);
    ~SettingsController() override;
    [[nodiscard]] bool captureEnabled() const noexcept;
    [[nodiscard]] QString captureStatus() const;
    [[nodiscard]] bool captureBlocked() const noexcept;
    [[nodiscard]] bool persistentHistory() const noexcept;
    void setCaptureEnabled(bool enabled);
    void storageFailed(const QString& message);

signals:
    void captureEnabledChanged();
    void captureStatusChanged();

private:
    void accessChanged(bool allowed);
    ClipboardController& clipboard_;
    bool captureEnabled_ = false;
    bool captureBlocked_ = false;
    QString captureStatus_ = QStringLiteral("Paused");
};

} // namespace kstack::desktop
