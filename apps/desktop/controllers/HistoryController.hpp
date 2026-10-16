/*
 * Author: Runor Ewhro
 * Description: exposes session history actions to QML: copying an entry or a
 *              converted form of it, removing one entry, and clearing all.
 */
#pragma once

#include "../models/EntrySubtexts.hpp"
#include "../models/HistoryListModel.hpp"
#include "../models/SearchResultsModel.hpp"

#include <kstack/core/HistoryService.hpp>

#include <QObject>

namespace kstack::desktop {

class HistoryController final : public QObject {
    Q_OBJECT

public:
    HistoryController(core::HistoryService& history, EntrySubtexts& subtexts,
                      HistoryListModel& historyModel, SearchResultsModel& searchModel,
                      QObject* parent = nullptr);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(qulonglong id);
    Q_INVOKABLE void copy(qulonglong id);
    // copies a converted form of an entry, such as a color as RGB
    Q_INVOKABLE void copyText(const QString& text);

private:
    core::HistoryService& history_;
    EntrySubtexts& subtexts_;
    HistoryListModel& historyModel_;
    SearchResultsModel& searchModel_;
};

} // namespace kstack::desktop
