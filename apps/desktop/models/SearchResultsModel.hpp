#pragma once

#include <kstack/core/SearchService.hpp>

#include <QAbstractListModel>
#include <QString>

namespace kstack::desktop {

class SearchResultsModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

public:
    explicit SearchResultsModel(const core::SearchService& search, QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QString query() const;
    void setQuery(const QString& query);
    void refresh();

signals:
    void queryChanged();

private:
    const core::SearchService& search_;
    QString query_;
    std::vector<core::ClipboardEntry> results_;
};

} // namespace kstack::desktop
