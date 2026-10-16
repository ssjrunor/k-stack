#pragma once

#include "../models/SearchResultsModel.hpp"

#include <QObject>
#include <QString>

namespace kstack::desktop {

class SearchController final : public QObject {
    Q_OBJECT

public:
    explicit SearchController(SearchResultsModel& results, QObject* parent = nullptr);
    Q_INVOKABLE void setQuery(const QString& query);

private:
    SearchResultsModel& results_;
};

} // namespace kstack::desktop
