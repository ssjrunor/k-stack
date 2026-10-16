/*
  Author: Runor Ewhro
  Description: starts and stops clipboard observation and forwards each
               accepted text item into the in-memory capture pipeline.
*/
#include "ClipboardController.hpp"

namespace kstack::desktop {

ClipboardController::ClipboardController(core::CapturePipeline& pipeline, HistoryListModel& historyModel,
                                         SearchResultsModel& searchModel)
    : pipeline_(pipeline), historyModel_(historyModel), searchModel_(searchModel),
      monitor_(platform::createClipboardMonitor()) {}

bool ClipboardController::start() {
    return monitor_->start([this](std::string text) {
        if (pipeline_.processText(text)) {
            historyModel_.refresh();
            searchModel_.refresh();
        }
    });
}

void ClipboardController::stop() { monitor_->stop(); }

} // namespace kstack::desktop
