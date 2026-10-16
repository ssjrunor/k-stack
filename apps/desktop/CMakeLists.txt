qt_add_executable(kstack_desktop
  main.cpp
  Application.cpp
  Application.hpp
  TrayMenu.cpp
  TrayMenu.hpp
  controllers/ClipboardController.cpp
  controllers/ClipboardController.hpp
  controllers/HistoryController.cpp
  controllers/HistoryController.hpp
  controllers/LinkTitleController.cpp
  controllers/LinkTitleController.hpp
  controllers/SearchController.cpp
  controllers/SearchController.hpp
  controllers/SettingsController.cpp
  controllers/SettingsController.hpp
  controllers/TranslationController.cpp
  controllers/TranslationController.hpp
  controllers/ViewOptionsController.cpp
  controllers/ViewOptionsController.hpp
  detection/CodeHighlighter.cpp
  detection/CodeHighlighter.hpp
  detection/HighlightJs.cpp
  detection/HighlightJs.hpp
  detection/HighlightJsTieBreaker.cpp
  detection/HighlightJsTieBreaker.hpp
  models/HistoryListModel.cpp
  models/HistoryListModel.hpp
  models/EntryPresentation.cpp
  models/EntryPresentation.hpp
  models/EntrySubtexts.cpp
  models/EntrySubtexts.hpp
  models/SearchResultsModel.cpp
  models/SearchResultsModel.hpp
  platform/PlatformFactory.hpp
  platform/PlatformUi.cpp
  platform/PlatformUi.hpp
  platform/SystemServices.cpp
  platform/SystemServices.hpp
)

qt_add_qml_module(kstack_desktop
  URI KStack
  VERSION 1.0
  RESOURCE_PREFIX "/"
  QML_FILES
    qml/Main.qml
    qml/windows/SearchPopup.qml
    qml/windows/HistoryWindow.qml
    qml/windows/SettingsWindow.qml
    qml/components/ClipboardItem.qml
    qml/components/SearchField.qml
    qml/components/CaptureStatus.qml
    qml/components/PanelFooter.qml
    qml/components/KindGlyph.qml
    qml/components/CategoryFilter.qml
    qml/components/PreviewPane.qml
    qml/components/CodeView.qml
    qml/components/EmptyState.qml
    qml/theme/Colors.qml
    qml/theme/Theme.qml
)

target_link_libraries(kstack_desktop PRIVATE
  kstack_core kstack_platform kstack_translation Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick Qt6::Widgets
)
if(KSTACK_HAS_PERSISTENCE)
  target_sources(kstack_desktop PRIVATE
    platform/macos/HistoryKeyStore.mm
    platform/macos/HistoryKeyStore.hpp)
  set_source_files_properties(platform/macos/HistoryKeyStore.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_compile_definitions(kstack_desktop PRIVATE KSTACK_HAS_PERSISTENCE=1)
  target_link_libraries(kstack_desktop PRIVATE kstack_storage)
  find_library(KSTACK_DESKTOP_SECURITY Security REQUIRED)
  target_link_libraries(kstack_desktop PRIVATE ${KSTACK_DESKTOP_SECURITY})
endif()
option(KSTACK_ENABLE_STRESS_PROFILE "Enable macOS clipboard stress verification and isolated profiles" OFF)
if(KSTACK_ENABLE_STRESS_PROFILE)
  if(NOT APPLE OR NOT KSTACK_HAS_PERSISTENCE OR NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "Stress profiles require a macOS Debug build with encrypted persistence")
  endif()
  target_sources(kstack_desktop PRIVATE
    stress/StressProfile.cpp
    stress/StressProfile.hpp)
  target_compile_definitions(kstack_desktop PRIVATE KSTACK_ENABLE_STRESS_PROFILE=1)
  # the stress reader keys SQLCipher directly, so it needs the codec API
  set_source_files_properties(stress/StressProfile.cpp PROPERTIES COMPILE_DEFINITIONS SQLITE_HAS_CODEC=1)
endif()
kstack_enable_warnings(kstack_desktop)
kstack_add_highlightjs(kstack_desktop)
# each platform supplies its own factory and overrides; shared code and QML
# only see the portable PlatformUi and SystemServices interfaces
if(APPLE)
  set(KSTACK_PLATFORM_SOURCES
    platform/macos/MacPlatformFactory.mm
    platform/macos/MacPlatformUi.mm
    platform/macos/MacPlatformUi.hpp
    platform/macos/MacSystemServices.mm
    platform/macos/MacSystemServices.hpp)
  target_sources(kstack_desktop PRIVATE ${KSTACK_PLATFORM_SOURCES})
  set_source_files_properties(${KSTACK_PLATFORM_SOURCES} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  find_library(KSTACK_DESKTOP_APPKIT AppKit REQUIRED)
  find_library(KSTACK_DESKTOP_EVENTKIT EventKit REQUIRED)
  target_link_libraries(kstack_desktop PRIVATE ${KSTACK_DESKTOP_APPKIT} ${KSTACK_DESKTOP_EVENTKIT})
  set_target_properties(kstack_desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/Info.plist"
  )
  # Keychain access is tied to the signing identity; the linker's ad-hoc
  # signature changes on every build, so each rebuild triggers an access prompt
  set(KSTACK_CODESIGN_IDENTITY "" CACHE STRING
    "macOS code signing identity for the app bundle; empty keeps the ad-hoc signature")
  if(KSTACK_CODESIGN_IDENTITY)
    # a self-signed identity works for Keychain access if every release keeps it;
    # only Developer ID builds can be notarized, which needs a secure timestamp
    set(kstack_codesign_timestamp "")
    if(KSTACK_CODESIGN_IDENTITY MATCHES "^Developer ID")
      set(kstack_codesign_timestamp "--timestamp")
    endif()
    add_custom_command(TARGET kstack_desktop POST_BUILD
      COMMAND codesign --force --sign "${KSTACK_CODESIGN_IDENTITY}"
        "$<$<NOT:$<CONFIG:Debug>>:--options=runtime;${kstack_codesign_timestamp}>"
        "$<TARGET_BUNDLE_DIR:kstack_desktop>"
      COMMAND_EXPAND_LISTS
      VERBATIM)
  endif()
else()
  target_sources(kstack_desktop PRIVATE platform/common/DefaultPlatformFactory.cpp)
endif()
install(TARGETS kstack_desktop RUNTIME DESTINATION bin BUNDLE DESTINATION .)
