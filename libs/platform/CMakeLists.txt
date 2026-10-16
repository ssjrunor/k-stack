add_library(kstack_platform STATIC
  common/PlatformFactory.cpp
)
target_include_directories(kstack_platform PUBLIC include PRIVATE common)
target_link_libraries(kstack_platform PUBLIC Qt6::Gui)
if(APPLE)
  target_sources(kstack_platform PRIVATE macos/MacClipboardMonitor.mm)
  set_source_files_properties(macos/MacClipboardMonitor.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  find_library(KSTACK_APPKIT_FRAMEWORK AppKit REQUIRED)
  target_link_libraries(kstack_platform PRIVATE ${KSTACK_APPKIT_FRAMEWORK})
else()
  target_sources(kstack_platform PRIVATE common/QtClipboardMonitor.cpp)
endif()
kstack_enable_warnings(kstack_platform)
