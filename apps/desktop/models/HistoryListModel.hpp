#pragma once

#include <kstack/core/HistoryService.hpp>

#include <QAbstractListModel>

namespace kstack::desktop {

class HistoryListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    explicit HistoryListModel(const core::HistoryService& history, QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    void refresh();

private:
    const core::HistoryService& history_;
};

} // namespace kstack::desktop
