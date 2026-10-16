/*
  Author: Runor Ewhro
  Description: projects core search results into QML roles, including the
               observed capture time for each session entry.
*/
#include "SearchResultsModel.hpp"

#include <QDateTime>

#include <chrono>

namespace kstack::desktop {

SearchResultsModel::SearchResultsModel(const core::SearchService& search, QObject* parent)
    : QAbstractListModel(parent), search_(search) {
    refresh();
}

int SearchResultsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(results_.size());
}

QVariant SearchResultsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const auto& entry = results_[static_cast<std::size_t>(index.row())];
    if (role == Qt::UserRole + 1) {
        return QString::fromStdString(entry.text);
    }
    if (role == Qt::UserRole + 2) {
        return QVariant::fromValue(static_cast<qulonglong>(entry.id));
    }
    if (role == Qt::UserRole + 3) {
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.capturedAt.time_since_epoch()).count();
        return QDateTime::fromMSecsSinceEpoch(milliseconds).toString(Qt::ISODate);
    }
    return {};
}

QHash<int, QByteArray> SearchResultsModel::roleNames() const {
    return {{Qt::UserRole + 1, "contentText"},
            {Qt::UserRole + 2, "entryId"},
            {Qt::UserRole + 3, "capturedAt"}};
}

QString SearchResultsModel::query() const { return query_; }

void SearchResultsModel::setQuery(const QString& query) {
    if (query_ == query) {
        return;
    }
    query_ = query;
    refresh();
    emit queryChanged();
}

void SearchResultsModel::refresh() {
    beginResetModel();
    results_ = search_.search(query_.toUtf8().toStdString());
    endResetModel();
}

} // namespace kstack::desktop
