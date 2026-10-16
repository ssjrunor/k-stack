/*
  Author: Runor Ewhro
  Description: connects the platform clipboard observer to core capture and
               refreshes desktop models after supported content is recorded.
*/
#pragma once

#include "../models/HistoryListModel.hpp"
#include "../models/SearchResultsModel.hpp"

#include <kstack/core/CapturePipeline.hpp>
#include <kstack/platform/IClipboardMonitor.hpp>

#include <functional>
#include <memory>
#include <QString>

namespace kstack::desktop {

class ClipboardController {
public:
    ClipboardController(core::CapturePipeline& pipeline, HistoryListModel& historyModel,
                        SearchResultsModel& searchModel);
    [[nodiscard]] bool start(platform::IClipboardMonitor::AccessCallback onAccessChanged = {});
    void stop();
    // runs after each accepted capture, once the models show it
    void setCaptureListener(std::function<void()> listener);
    void setStorageErrorListener(std::function<void(const QString&)> listener);

private:
    core::CapturePipeline& pipeline_;
    HistoryListModel& historyModel_;
    SearchResultsModel& searchModel_;
    std::unique_ptr<platform::IClipboardMonitor> monitor_;
    std::function<void()> captureListener_;
    std::function<void(const QString&)> storageErrorListener_;
};

} // namespace kstack::desktop
