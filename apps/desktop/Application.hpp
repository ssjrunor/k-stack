/*
 * Author: Runor Ewhro
 * Description: owns the services, models, controllers, platform layer, and
 *              QML engine that make up the running desktop application.
 */
#pragma once

#include "controllers/ClipboardController.hpp"
#include "controllers/HistoryController.hpp"
#include "controllers/LinkTitleController.hpp"
#include "controllers/SearchController.hpp"
#include "controllers/SettingsController.hpp"
#include "controllers/TranslationController.hpp"
#include "controllers/ViewOptionsController.hpp"
#include "detection/CodeHighlighter.hpp"
#include "detection/HighlightJs.hpp"
#include "detection/HighlightJsTieBreaker.hpp"
#include "models/EntrySubtexts.hpp"
#include "models/HistoryListModel.hpp"
#include "models/SearchResultsModel.hpp"
#include "TrayMenu.hpp"
#include "platform/PlatformUi.hpp"
#include "platform/SystemServices.hpp"

#include <kstack/core/CapturePipeline.hpp>
#include <kstack/core/ClipboardService.hpp>
#include <kstack/core/HistoryService.hpp>
#include <kstack/core/SearchService.hpp>
#ifdef KSTACK_HAS_PERSISTENCE
#include <kstack/storage/Database.hpp>
#include <kstack/storage/EntryRepository.hpp>
#endif

#include <QQmlApplicationEngine>

#include <memory>
#include <QQuickWindow>
#include <QString>

#include <string>

namespace kstack::desktop {

class Application {
public:
    Application();
    [[nodiscard]] bool start(const QString& stressDirectory = {},
                             const std::string& stressKeyService = {});

private:
    // one highlight.js serves language tie-breaks and code colouring
    HighlightJs highlightJs_;
    HighlightJsTieBreaker tieBreaker_;
    CodeHighlighter codeHighlighter_;
    core::HistoryService history_;
#ifdef KSTACK_HAS_PERSISTENCE
    std::unique_ptr<storage::Database> database_;
    std::unique_ptr<storage::EntryRepository> store_;
#endif
    core::ClipboardService clipboardService_;
    core::CapturePipeline capturePipeline_;
    core::SearchService searchService_;
    EntrySubtexts subtexts_;
    HistoryListModel historyModel_;
    SearchResultsModel searchModel_;
    ClipboardController clipboardController_;
    HistoryController historyController_;
    SearchController searchController_;
    SettingsController settingsController_;
    ViewOptionsController viewOptions_;
    TranslationController translation_;
    LinkTitleController linkTitles_;
    std::unique_ptr<PlatformUi> platformUi_;
    std::unique_ptr<SystemServices> systemServices_;
    QQmlApplicationEngine engine_;
    TrayMenu trayMenu_;
    QQuickWindow* historyWindow_ = nullptr;
};

} // namespace kstack::desktop
