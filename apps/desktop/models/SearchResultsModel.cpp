/*
 * Author: Runor Ewhro
 * Description: runs the session search, presents each match, counts the
 *              categories present, applies the category filter, and orders
 *              rows so each group's section label stays contiguous.
 */
#include "SearchResultsModel.hpp"

#include <QDateTime>

#include <algorithm>
#include <utility>

namespace kstack::desktop {

SearchResultsModel::SearchResultsModel(const core::SearchService& search,
                                       const EntrySubtexts& subtexts, QObject* parent)
    : QAbstractListModel(parent), search_(search), subtexts_(subtexts) {
    connect(&subtexts_, &EntrySubtexts::changed, this, &SearchResultsModel::subtextChanged);
    refresh();
}

int SearchResultsModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant SearchResultsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const Row& row = rows_[static_cast<std::size_t>(index.row())];
    const EntryPresentation& p = row.presentation;
    switch (role) {
    case ContentTextRole:
        return QString::fromStdString(row.entry.text);
    case EntryIdRole:
        return QVariant::fromValue(static_cast<qulonglong>(row.entry.id));
    case CapturedAtRole:
        return p.capturedAt.toString(Qt::ISODate);
    case KindRole:
        return p.kind;
    case KindLabelRole:
        return p.kindLabel;
    case CategoryRole:
        return p.category;
    case CategoryLabelRole:
        return p.categoryLabel;
    case TitleRole:
        return p.title;
    case TitleTailRole:
        return p.titleTail;
    case DetailRole:
        return p.detail;
    case ActionLabelRole:
        return p.actionLabel;
    case ActionUrlRole:
        return p.actionUrl;
    case ActionTextRole:
        return p.actionText;
    case ActionKindRole:
        return p.actionKind;
    case SwatchRole:
        return p.swatch.isValid() ? QVariant(p.swatch) : QVariant(QColor(Qt::transparent));
    case SensitiveRole:
        return p.sensitive;
    case MonospaceRole:
        return p.monospace;
    case SectionRole:
        return row.section;
    case WhenRole:
        return p.when.isValid() ? p.when.toString(Qt::ISODate) : QString();
    case AllDayRole:
        return p.allDay;
    case WhenLabelRole:
        return p.whenLabel;
    case SubtextRole:
        return subtexts_.value(row.entry.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> SearchResultsModel::roleNames() const {
    return {{ContentTextRole, "contentText"},     {EntryIdRole, "entryId"},
            {CapturedAtRole, "capturedAt"},       {KindRole, "kind"},
            {KindLabelRole, "kindLabel"},         {CategoryRole, "category"},
            {CategoryLabelRole, "categoryLabel"}, {TitleRole, "title"},
            {TitleTailRole, "titleTail"},         {DetailRole, "detail"},
            {ActionLabelRole, "actionLabel"},     {ActionUrlRole, "actionUrl"},
            {ActionTextRole, "actionText"},       {ActionKindRole, "actionKind"},
            {SwatchRole, "swatch"},               {SensitiveRole, "sensitive"},
            {MonospaceRole, "monospace"},         {SectionRole, "section"},
            {WhenRole, "when"},                   {AllDayRole, "allDay"},
            {WhenLabelRole, "whenLabel"},         {SubtextRole, "subtext"}};
}

// a subtext can arrive after its row is shown, such as a finished
// translation, so only that row updates
void SearchResultsModel::subtextChanged(quint64 id) {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].entry.id == id) {
            const QModelIndex changed = index(static_cast<int>(i));
            emit dataChanged(changed, changed, {SubtextRole});
            return;
        }
    }
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

QString SearchResultsModel::category() const { return category_; }

void SearchResultsModel::setCategory(const QString& category) {
    if (category_ == category) {
        return;
    }
    category_ = category;
    refresh();
    emit categoryChanged();
}

QString SearchResultsModel::groupBy() const { return groupBy_; }

void SearchResultsModel::setGroupBy(const QString& groupBy) {
    if (groupBy_ == groupBy) {
        return;
    }
    groupBy_ = groupBy;
    refresh();
    emit groupByChanged();
}

QVariantList SearchResultsModel::categories() const { return categories_; }

int SearchResultsModel::matchCount() const { return matchCount_; }

void SearchResultsModel::refresh() {
    auto matches = search_.search(query_.toUtf8().toStdString());
    const auto& order = entryCategories();
    QHash<QString, int> counts;
    std::vector<Row> rows;
    rows.reserve(matches.size());
    for (auto& entry : matches) {
        auto presentation = presentEntry(entry);
        Row row{std::move(entry), std::move(presentation), {}};
        ++counts[row.presentation.category];
        if (category_.isEmpty() || row.presentation.category == category_) {
            rows.push_back(std::move(row));
        }
    }

    // a filter for a category that no longer has matches falls back to all,
    // so the panel never shows an empty list behind a stale chip
    if (!category_.isEmpty() && !counts.contains(category_)) {
        category_.clear();
        emit categoryChanged();
        refresh();
        return;
    }

    const auto rank = [&order](const QString& id) {
        for (int i = 0; i < order.size(); ++i) {
            if (order[i].id == id) {
                return i;
            }
        }
        return static_cast<int>(order.size());
    };
    const QDateTime now = QDateTime::currentDateTime();
    if (groupBy_ == QLatin1String("kind")) {
        std::stable_sort(rows.begin(), rows.end(), [&rank](const Row& left, const Row& right) {
            return rank(left.presentation.category) < rank(right.presentation.category);
        });
    }
    for (Row& row : rows) {
        if (groupBy_ == QLatin1String("kind")) {
            row.section = row.presentation.categoryLabel;
        } else if (groupBy_ == QLatin1String("time") && query_.isEmpty()) {
            // search results are ordered by relevance, not time, so time
            // groups apply only to the unsearched list
            row.section = timeGroup(row.presentation.capturedAt, now);
        }
    }

    beginResetModel();
    rows_ = std::move(rows);
    endResetModel();

    categories_.clear();
    for (const auto& category : order) {
        if (const int count = counts.value(category.id); count > 0) {
            categories_.append(QVariantMap{
                {QStringLiteral("id"), category.id},
                {QStringLiteral("label"), category.label},
                {QStringLiteral("count"), count},
            });
        }
    }
    matchCount_ = static_cast<int>(matches.size());
    emit categoriesChanged();
}

} // namespace kstack::desktop
