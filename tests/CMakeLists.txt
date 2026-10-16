# Author: Runor Ewhro
# Description: builds core contract checks and the macOS clipboard integration
#              check used during P0 capture validation.
add_executable(kstack_core_tests unit/CoreTests.cpp)
target_link_libraries(kstack_core_tests PRIVATE kstack_core)
kstack_enable_warnings(kstack_core_tests)
add_test(NAME core COMMAND kstack_core_tests)

if(APPLE)
  add_executable(kstack_mac_clipboard_tests integration/MacClipboardMonitorTests.mm)
  set_source_files_properties(integration/MacClipboardMonitorTests.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_include_directories(kstack_mac_clipboard_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/platform/macos")
  target_link_libraries(kstack_mac_clipboard_tests PRIVATE kstack_platform kstack_core)
  kstack_enable_warnings(kstack_mac_clipboard_tests)
  add_test(NAME mac_clipboard COMMAND kstack_mac_clipboard_tests)
  set_tests_properties(mac_clipboard PROPERTIES SKIP_RETURN_CODE 77)
endif()
