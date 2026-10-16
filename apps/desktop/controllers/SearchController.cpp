#include "SearchController.hpp"

namespace kstack::desktop {

SearchController::SearchController(SearchResultsModel& results, QObject* parent)
    : QObject(parent), results_(results) {}

void SearchController::setQuery(const QString& query) { results_.setQuery(query); }

} // namespace kstack::desktop
