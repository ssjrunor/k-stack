add_library(kstack_core STATIC
  src/ClipboardService.cpp
  src/CapturePipeline.cpp
  src/HistoryService.cpp
  src/SearchService.cpp
  src/RetentionPolicy.cpp
)
target_include_directories(kstack_core PUBLIC include)
target_link_libraries(kstack_core PUBLIC kstack_algorithms)
kstack_enable_warnings(kstack_core)
