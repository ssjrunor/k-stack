/*
 * Author: Runor Ewhro
 * Description: projects matching session entries into QML roles, filtered by
 *              category, ordered and labelled for the chosen grouping, with
 *              per-category counts for the panel's filter.
 */
#pragma once

#include "EntryPresentation.hpp"
#include "EntrySubtexts.hpp"

#include <kstack/core/SearchService.hpp>

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>

#include <vector>

namespace kstack::desktop {

class SearchResultsModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)
    Q_PROPERTY(QString groupBy READ groupBy WRITE setGroupBy NOTIFY groupByChanged)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(int matchCount READ matchCount NOTIFY categoriesChanged)

public:
    enum Role {
        ContentTextRole = Qt::UserRole + 1,
        EntryIdRole,
        CapturedAtRole,
        KindRole,
        KindLabelRole,
        CategoryRole,
        CategoryLabelRole,
        TitleRole,
        TitleTailRole,
        DetailRole,
        ActionLabelRole,
        ActionUrlRole,
        ActionTextRole,
        ActionKindRole,
        SwatchRole,
        SensitiveRole,
        MonospaceRole,
        SectionRole,
        WhenRole,
        AllDayRole,
        WhenLabelRole,
        SubtextRole,
    };

    SearchResultsModel(const core::SearchService& search, const EntrySubtexts& subtexts,
                       QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QString query() const;
    void setQuery(const QString& query);
    [[nodiscard]] QString category() const;
    void setCategory(const QString& category);
    [[nodiscard]] QString groupBy() const;
    void setGroupBy(const QString& groupBy);
    // categories present among the matches, each as {id, label, count}
    [[nodiscard]] QVariantList categories() const;
    // matches before the category filter, for the "All" chip
    [[nodiscard]] int matchCount() const;
    // also called when the panel opens, so time groups and relative dates
    // are measured from now
    Q_INVOKABLE void refresh();

signals:
    void queryChanged();
    void categoryChanged();
    void groupByChanged();
    void categoriesChanged();

private:
    struct Row {
        core::ClipboardEntry entry;
        EntryPresentation presentation;
        QString section;
    };

    void subtextChanged(quint64 id);

    const core::SearchService& search_;
    const EntrySubtexts& subtexts_;
    QString query_;
    QString category_;
    QString groupBy_ = QStringLiteral("time");
    std::vector<Row> rows_;
    QVariantList categories_;
    int matchCount_ = 0;
};

} // namespace kstack::desktop
