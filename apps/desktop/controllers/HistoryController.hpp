#pragma once

#include "../models/HistoryListModel.hpp"
#include "../models/SearchResultsModel.hpp"

#include <kstack/core/HistoryService.hpp>

#include <QObject>

namespace kstack::desktop {

class HistoryController final : public QObject {
    Q_OBJECT

public:
    HistoryController(core::HistoryService& history, HistoryListModel& historyModel,
                      SearchResultsModel& searchModel, QObject* parent = nullptr);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(qulonglong id);
    Q_INVOKABLE void copy(qulonglong id);

private:
    core::HistoryService& history_;
    HistoryListModel& historyModel_;
    SearchResultsModel& searchModel_;
};

} // namespace kstack::desktop
