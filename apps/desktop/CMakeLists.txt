qt_add_executable(kstack_desktop
  main.cpp
  Application.cpp
  Application.hpp
  controllers/ClipboardController.cpp
  controllers/ClipboardController.hpp
  controllers/HistoryController.cpp
  controllers/HistoryController.hpp
  controllers/SearchController.cpp
  controllers/SearchController.hpp
  controllers/SettingsController.cpp
  controllers/SettingsController.hpp
  models/HistoryListModel.cpp
  models/HistoryListModel.hpp
  models/SearchResultsModel.cpp
  models/SearchResultsModel.hpp
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
    qml/components/EmptyState.qml
    qml/theme/Colors.qml
    qml/theme/Theme.qml
)

target_link_libraries(kstack_desktop PRIVATE
  kstack_core kstack_platform Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Widgets
)
kstack_enable_warnings(kstack_desktop)
if(APPLE)
  target_sources(kstack_desktop PRIVATE macos/NativeMaterial.mm macos/NativeMaterial.hpp)
  set_source_files_properties(macos/NativeMaterial.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  find_library(KSTACK_DESKTOP_APPKIT AppKit REQUIRED)
  target_link_libraries(kstack_desktop PRIVATE ${KSTACK_DESKTOP_APPKIT})
  set_target_properties(kstack_desktop PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist"
  )
endif()
install(TARGETS kstack_desktop RUNTIME DESTINATION bin BUNDLE DESTINATION .)
