## Author: Runor Ewhro
## Description: locates the desktop dependencies and preserves CMake version compatibility.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)
find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3)
  add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()
qt_standard_project_setup(REQUIRES 6.4)
