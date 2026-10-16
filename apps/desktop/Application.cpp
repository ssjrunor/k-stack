/*
  Author: Runor Ewhro
  Description: wires the desktop services, the platform layer, the QML
               history window, and the tray control into one running
               clipboard application.
*/
#include "Application.hpp"

#include "platform/PlatformFactory.hpp"
#ifdef KSTACK_HAS_PERSISTENCE
#include "platform/macos/HistoryKeyStore.hpp"
#endif

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QQmlContext>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QUrl>

#include <algorithm>
#include <exception>

namespace kstack::desktop {

Application::Application()
    : tieBreaker_(highlightJs_), codeHighlighter_(highlightJs_), clipboardService_(history_), capturePipeline_(clipboardService_), searchService_(history_),
      historyModel_(history_), searchModel_(searchService_, subtexts_),
      clipboardController_(capturePipeline_, historyModel_, searchModel_),
      historyController_(history_, subtexts_, historyModel_, searchModel_),
      searchController_(searchModel_), settingsController_(clipboardController_),
      translation_(history_, subtexts_), linkTitles_(history_, subtexts_), platformUi_(createPlatformUi()),
      systemServices_(createSystemServices()),
      trayMenu_(history_, historyModel_, historyController_, settingsController_) {
    clipboardController_.setStorageErrorListener([this](const QString& message) {
        settingsController_.storageFailed(message);
    });
    // close calls on a code snippet's language go to highlight.js
    history_.setCodeLanguageTieBreaker([this](std::string_view text, const auto& first, const auto& second) {
        return tieBreaker_.choose(text, first, second);
    });
    // each new entry may earn a subtext: code shows its language, text in
    // another language its translation, and a link its page title
    clipboardController_.setCaptureListener([this] {
        const auto& entries = history_.entries();
        subtexts_.retain(entries);
        if (entries.empty()) {
            return;
        }
        const core::ClipboardEntry& entry = entries.front();
        if (!entry.codeLanguage.empty()) {
            subtexts_.set(entry.id, QString::fromStdString(entry.codeLanguage));
        }
        translation_.entryCaptured(entry);
        linkTitles_.entryCaptured(entry);
    });
}

// load the history window hidden, expose it through the tray menu, then start
// capture; a failed monitor start opens the window so the state is visible
bool Application::start(const QString& stressDirectory, const std::string& stressKeyService) {
    QApplication::setQuitOnLastWindowClosed(false);
#ifndef KSTACK_HAS_PERSISTENCE
    Q_UNUSED(stressDirectory)
    Q_UNUSED(stressKeyService)
#endif
#ifdef KSTACK_HAS_PERSISTENCE
    try {
        const QString directory = stressDirectory.isEmpty()
                                      ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                      : stressDirectory;
        if (directory.isEmpty() || !QDir().mkpath(directory)) {
            throw std::runtime_error("Cannot create the history directory");
        }
        if (!QFile::setPermissions(directory, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                 QFileDevice::ExeOwner)) {
            throw std::runtime_error("Cannot restrict the history directory");
        }
        const QString path = QDir(directory).filePath(QStringLiteral("history.db"));
        const bool existingHistory = QFileInfo::exists(path) || QFileInfo::exists(path + QStringLiteral("-wal")) ||
                                     QFileInfo::exists(path + QStringLiteral("-shm"));
        auto key = stressKeyService.empty() ? historyDatabaseKey(existingHistory)
                                           : historyDatabaseKey(existingHistory, stressKeyService);
        try {
            database_ = std::make_unique<storage::Database>(path.toStdString(), key);
        } catch (...) {
            std::fill(key.begin(), key.end(), 0);
            throw;
        }
        std::fill(key.begin(), key.end(), 0);
        if (!QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
            throw std::runtime_error("Cannot restrict the history database file");
        }
        store_ = std::make_unique<storage::EntryRepository>(*database_);
        history_.attachStore(*store_);
        historyModel_.refresh();
        searchModel_.refresh();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, QStringLiteral("History unavailable"),
                              QString::fromUtf8(error.what()));
        return false;
    }
#endif
    engine_.rootContext()->setContextProperty("historyModel", &historyModel_);
    engine_.rootContext()->setContextProperty("searchModel", &searchModel_);
    engine_.rootContext()->setContextProperty("historyController", &historyController_);
    engine_.rootContext()->setContextProperty("searchController", &searchController_);
    engine_.rootContext()->setContextProperty("settingsController", &settingsController_);
    engine_.rootContext()->setContextProperty("viewOptions", &viewOptions_);
    engine_.rootContext()->setContextProperty("translationController", &translation_);
    engine_.rootContext()->setContextProperty("codeHighlighter", &codeHighlighter_);
    // QML always gets both platform objects and reads their capability
    // flags, so it never needs to know which system it runs on
    engine_.rootContext()->setContextProperty("platformUi", platformUi_.get());
    engine_.rootContext()->setContextProperty("systemServices", systemServices_.get());
    engine_.rootContext()->setContextProperty("summonMode",
                                              QSystemTrayIcon::isSystemTrayAvailable());
    engine_.load(QUrl(QStringLiteral("qrc:/KStack/qml/Main.qml")));
    if (engine_.rootObjects().isEmpty()) {
        return false;
    }
    historyWindow_ = qobject_cast<QQuickWindow*>(engine_.rootObjects().front());
    if (!historyWindow_) {
        return false;
    }
    systemServices_->setWindow(historyWindow_);
    // a summoned panel gets the system material when the platform has one;
    // the corner radius comes from QML so the material and rows stay concentric
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        platformUi_->attachMaterial(historyWindow_, historyWindow_->property("cornerRadius").toReal());
    }

    QObject::connect(&trayMenu_, &TrayMenu::openHistoryRequested, &trayMenu_, [this] {
        historyWindow_->show();
        historyWindow_->raise();
        historyWindow_->requestActivate();
    });
    if (!trayMenu_.show()) {
        QApplication::setQuitOnLastWindowClosed(true);
        historyWindow_->show();
    }
    settingsController_.setCaptureEnabled(true);
    if (!settingsController_.captureEnabled()) {
        historyWindow_->show();
        historyWindow_->raise();
        historyWindow_->requestActivate();
    }
#ifdef KSTACK_ENABLE_STRESS_PROFILE
    if (!stressDirectory.isEmpty()) {
        QFile ready(QDir(stressDirectory).filePath(QStringLiteral("ready")));
        if (!ready.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
            ready.write(settingsController_.captureEnabled() ? "capturing" : "unavailable") < 0) {
            return false;
        }
    }
#endif
    return true;
}

} // namespace kstack::desktop
