#include "HistoryController.hpp"

#include <QClipboard>
#include <QGuiApplication>

#include <algorithm>

namespace kstack::desktop {

HistoryController::HistoryController(core::HistoryService& history, HistoryListModel& historyModel,
                                     SearchResultsModel& searchModel, QObject* parent)
    : QObject(parent), history_(history), historyModel_(historyModel), searchModel_(searchModel) {}

void HistoryController::clear() {
    history_.clear();
    historyModel_.refresh();
    searchModel_.refresh();
}

void HistoryController::remove(qulonglong id) {
    if (history_.remove(id)) {
        historyModel_.refresh();
        searchModel_.refresh();
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
