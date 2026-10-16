#include "HistoryListModel.hpp"

namespace kstack::desktop {

HistoryListModel::HistoryListModel(const core::HistoryService& history, QObject* parent)
    : QAbstractListModel(parent), history_(history) {}

int HistoryListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(history_.entries().size());
}

QVariant HistoryListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const auto& entry = history_.entries()[static_cast<std::size_t>(index.row())];
    if (role == Qt::UserRole + 1) {
        return QString::fromStdString(entry.text);
    }
    if (role == Qt::UserRole + 2) {
        return QVariant::fromValue(static_cast<qulonglong>(entry.id));
    }
    return {};
}

QHash<int, QByteArray> HistoryListModel::roleNames() const {
    return {{Qt::UserRole + 1, "contentText"}, {Qt::UserRole + 2, "entryId"}};
}

void HistoryListModel::refresh() {
    beginResetModel();
    endResetModel();
}

} // namespace kstack::desktop
