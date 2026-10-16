#pragma once

#include "controllers/ClipboardController.hpp"
#include "controllers/HistoryController.hpp"
#include "controllers/SearchController.hpp"
#include "controllers/SettingsController.hpp"
#include "models/HistoryListModel.hpp"
#include "models/SearchResultsModel.hpp"

#include <kstack/core/CapturePipeline.hpp>
#include <kstack/core/ClipboardService.hpp>
#include <kstack/core/HistoryService.hpp>
#include <kstack/core/SearchService.hpp>

#include <QQmlApplicationEngine>

#include <memory>
#include <QMenu>
#include <QQuickWindow>
#include <QSystemTrayIcon>

namespace kstack::desktop {

class Application {
public:
    Application();
    [[nodiscard]] bool start();

private:
    core::HistoryService history_;
    core::ClipboardService clipboardService_;
    core::CapturePipeline capturePipeline_;
    core::SearchService searchService_;
    HistoryListModel historyModel_;
    SearchResultsModel searchModel_;
    ClipboardController clipboardController_;
    HistoryController historyController_;
    SearchController searchController_;
    SettingsController settingsController_;
    QQmlApplicationEngine engine_;
    QMenu trayMenu_;
    QSystemTrayIcon trayIcon_;
    QQuickWindow* historyWindow_ = nullptr;
    std::unique_ptr<QObject> materialBridge_;
};

} // namespace kstack::desktop
