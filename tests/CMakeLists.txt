# Author: Runor Ewhro
# Description: builds core contract checks and the macOS clipboard integration
#              check used during P0 capture validation.
add_executable(kstack_core_tests unit/CoreTests.cpp)
target_link_libraries(kstack_core_tests PRIVATE kstack_core)
kstack_enable_warnings(kstack_core_tests)
add_test(NAME core COMMAND kstack_core_tests)

if(KSTACK_HAS_PERSISTENCE)
  add_executable(kstack_encrypted_history_tests integration/EncryptedHistoryTests.cpp)
  target_compile_definitions(kstack_encrypted_history_tests PRIVATE SQLITE_HAS_CODEC=1)
  target_link_libraries(kstack_encrypted_history_tests PRIVATE kstack_storage)
  kstack_enable_warnings(kstack_encrypted_history_tests)
  add_test(NAME encrypted_history COMMAND kstack_encrypted_history_tests)
endif()

# language choice and routing always run; detection and translation run
# when they are built in, and translation also needs installed models
add_executable(kstack_translation_tests unit/TranslationTests.cpp)
target_link_libraries(kstack_translation_tests PRIVATE kstack_translation)
if(KSTACK_ENABLE_TRANSLATION)
  target_compile_definitions(kstack_translation_tests PRIVATE KSTACK_TRANSLATION_BUILT=1)
endif()
kstack_enable_warnings(kstack_translation_tests)
add_test(NAME translation COMMAND kstack_translation_tests)
set_tests_properties(translation PROPERTIES SKIP_RETURN_CODE 77)

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

# highlight.js as the tie-breaker, run in Qt's JavaScript engine as in the
# app; KSTACK_TIEBREAK_EVAL can point it at a dump from the tool below
add_executable(kstack_highlightjs_tests
  integration/HighlightJsTieBreakerTests.cpp
  ${PROJECT_SOURCE_DIR}/apps/desktop/detection/CodeHighlighter.cpp
  ${PROJECT_SOURCE_DIR}/apps/desktop/detection/HighlightJs.cpp
  ${PROJECT_SOURCE_DIR}/apps/desktop/detection/HighlightJsTieBreaker.cpp)
target_include_directories(kstack_highlightjs_tests PRIVATE ${PROJECT_SOURCE_DIR}/apps/desktop)
target_link_libraries(kstack_highlightjs_tests PRIVATE codelang::codelang Qt6::Core Qt6::Gui Qt6::Qml)
set_target_properties(kstack_highlightjs_tests PROPERTIES AUTOMOC ON)
kstack_add_highlightjs(kstack_highlightjs_tests)
kstack_enable_warnings(kstack_highlightjs_tests)
add_test(NAME highlightjs COMMAND kstack_highlightjs_tests)

# measures how reliably code is recognised as code and prose is not; run by
# hand, see the file
add_executable(kstack_code_kind_eval tools/CodeKindEval.cpp)
target_link_libraries(kstack_code_kind_eval PRIVATE kstack_core)
kstack_enable_warnings(kstack_code_kind_eval)

# link titles parse offline; KSTACK_LINK_TITLE_URL also fetches a real page
add_executable(kstack_link_title_tests
  integration/LinkTitleTests.cpp
  ${PROJECT_SOURCE_DIR}/apps/desktop/controllers/LinkTitleController.cpp
  ${PROJECT_SOURCE_DIR}/apps/desktop/models/EntrySubtexts.cpp)
target_include_directories(kstack_link_title_tests PRIVATE ${PROJECT_SOURCE_DIR}/apps/desktop)
target_link_libraries(kstack_link_title_tests PRIVATE kstack_core Qt6::Core Qt6::Gui Qt6::Network)
set_target_properties(kstack_link_title_tests PROPERTIES AUTOMOC ON)
kstack_enable_warnings(kstack_link_title_tests)
add_test(NAME link_title COMMAND kstack_link_title_tests)
