/*
 * Author: Runor Ewhro
 * Description: copies entries to the system clipboard and removes them from
 *              history, refreshing the models that show it.
 */
#include "HistoryController.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>

#include <algorithm>
#include <exception>

namespace kstack::desktop {

HistoryController::HistoryController(core::HistoryService& history, EntrySubtexts& subtexts,
                                     HistoryListModel& historyModel, SearchResultsModel& searchModel,
                                     QObject* parent)
    : QObject(parent), history_(history), subtexts_(subtexts), historyModel_(historyModel),
      searchModel_(searchModel) {}

void HistoryController::clear() {
    try {
        history_.clear();
    } catch (const std::exception& error) {
        QMessageBox::warning(nullptr, QStringLiteral("History could not be cleared"),
                             QString::fromUtf8(error.what()));
        return;
    }
    subtexts_.clear();
    historyModel_.refresh();
    searchModel_.refresh();
}

void HistoryController::remove(qulonglong id) {
    bool removed = false;
    try {
        removed = history_.remove(id);
    } catch (const std::exception& error) {
        QMessageBox::warning(nullptr, QStringLiteral("Item could not be removed"),
                             QString::fromUtf8(error.what()));
        return;
    }
    if (removed) {
        subtexts_.remove(id);
        historyModel_.refresh();
        searchModel_.refresh();
    }
}

void HistoryController::copyText(const QString& text) {
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
    }
}

void HistoryController::copy(qulonglong id) {
    const auto& entries = history_.entries();
    const auto found = std::find_if(entries.begin(), entries.end(), [id](const auto& entry) {
        return entry.id == id;
    });
    if (found != entries.end()) {
        QGuiApplication::clipboard()->setText(QString::fromStdString(found->text));
    }
}

} // namespace kstack::desktop
