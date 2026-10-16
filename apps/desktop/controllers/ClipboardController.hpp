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

#include <memory>

namespace kstack::desktop {

class ClipboardController {
public:
    ClipboardController(core::CapturePipeline& pipeline, HistoryListModel& historyModel,
                        SearchResultsModel& searchModel);
    [[nodiscard]] bool start();
    void stop();

private:
    core::CapturePipeline& pipeline_;
    HistoryListModel& historyModel_;
    SearchResultsModel& searchModel_;
    std::unique_ptr<platform::IClipboardMonitor> monitor_;
};

} // namespace kstack::desktop
