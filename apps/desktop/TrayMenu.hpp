/*
 * Author: Runor Ewhro
 * Description: owns the tray icon and its menu: capture status, the latest
 *              session entries for copying in one click or with the first
 *              nine number shortcuts, and the app's background controls.
 */
#pragma once

#include "controllers/HistoryController.hpp"
#include "controllers/SettingsController.hpp"
#include "models/HistoryListModel.hpp"

#include <kstack/core/HistoryService.hpp>

#include <QList>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>

class QAction;

namespace kstack::desktop {

class TrayMenu final : public QObject {
    Q_OBJECT

public:
    TrayMenu(const core::HistoryService& history, HistoryListModel& historyModel,
             HistoryController& historyController, SettingsController& settings,
             QObject* parent = nullptr);
    // shows the icon when the desktop has a system tray; returns false
    // when it does not, so the caller can fall back to an ordinary window
    bool show();

signals:
    void openHistoryRequested();

private:
    void rebuildRecent();
    void showCaptureState();
    void confirmCopy();

    const core::HistoryService& history_;
    HistoryController& historyController_;
    SettingsController& settings_;
    QMenu menu_;
    QSystemTrayIcon icon_;
    QAction* status_ = nullptr;
    QAction* recentEnd_ = nullptr;
    QAction* capture_ = nullptr;
    QList<QAction*> recent_;
    QTimer confirmTimer_;
};

} // namespace kstack::desktop
