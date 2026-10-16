/*
  Author: Runor Ewhro
  Description: starts and stops clipboard observation and forwards each
               accepted text item into the capture pipeline.
*/
#include "ClipboardController.hpp"

#include <exception>
#include <utility>

namespace kstack::desktop {

ClipboardController::ClipboardController(core::CapturePipeline& pipeline, HistoryListModel& historyModel,
                                         SearchResultsModel& searchModel)
    : pipeline_(pipeline), historyModel_(historyModel), searchModel_(searchModel),
      monitor_(platform::createClipboardMonitor()) {}

bool ClipboardController::start(platform::IClipboardMonitor::AccessCallback onAccessChanged) {
    return monitor_->start([this](std::string text) {
        bool captured = false;
        try {
            captured = pipeline_.processText(text);
        } catch (const std::exception& error) {
            if (storageErrorListener_) {
                storageErrorListener_(QString::fromUtf8(error.what()));
            }
            return;
        }
        if (captured) {
            historyModel_.refresh();
            searchModel_.refresh();
            if (captureListener_) {
                captureListener_();
            }
        }
    }, std::move(onAccessChanged));
}

void ClipboardController::stop() { monitor_->stop(); }

void ClipboardController::setCaptureListener(std::function<void()> listener) {
    captureListener_ = std::move(listener);
}

void ClipboardController::setStorageErrorListener(std::function<void(const QString&)> listener) {
    storageErrorListener_ = std::move(listener);
}

} // namespace kstack::desktop
