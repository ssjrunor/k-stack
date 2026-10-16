## Author: Runor Ewhro
## Description: locates the desktop dependencies and preserves CMake version compatibility.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Qml Quick Widgets)

# SQLCipher is the only library allowed to open persistent clipboard history.
# AUTO keeps P0 builds available where it has not been installed yet.
set(KSTACK_PERSISTENCE "AUTO" CACHE STRING "Encrypted history: AUTO, ON, or OFF")
set_property(CACHE KSTACK_PERSISTENCE PROPERTY STRINGS AUTO ON OFF)
if(NOT KSTACK_PERSISTENCE MATCHES "^(AUTO|ON|OFF)$")
  message(FATAL_ERROR "KSTACK_PERSISTENCE must be AUTO, ON, or OFF")
endif()
if(NOT KSTACK_PERSISTENCE STREQUAL "OFF" AND APPLE)
  set(KSTACK_SQLCIPHER_ROOTS
    /opt/homebrew/opt/sqlcipher
    /usr/local/opt/sqlcipher
    $ENV{HOMEBREW_PREFIX}/opt/sqlcipher)
  find_library(KSTACK_SQLCIPHER_LIBRARY NAMES sqlcipher
    HINTS ${KSTACK_SQLCIPHER_ROOTS} PATH_SUFFIXES lib)
  find_path(KSTACK_SQLCIPHER_INCLUDE_DIR NAMES sqlite3.h
    HINTS ${KSTACK_SQLCIPHER_ROOTS} PATH_SUFFIXES include/sqlcipher NO_DEFAULT_PATH)
  if(KSTACK_SQLCIPHER_LIBRARY AND KSTACK_SQLCIPHER_INCLUDE_DIR)
    add_library(SQLCipher::SQLCipher UNKNOWN IMPORTED)
    set_target_properties(SQLCipher::SQLCipher PROPERTIES
      IMPORTED_LOCATION "${KSTACK_SQLCIPHER_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${KSTACK_SQLCIPHER_INCLUDE_DIR}")
    set(KSTACK_HAS_PERSISTENCE ON)
  endif()
endif()
if(KSTACK_PERSISTENCE STREQUAL "ON" AND NOT KSTACK_HAS_PERSISTENCE)
  message(FATAL_ERROR "Encrypted history needs SQLCipher and a platform key store (macOS: brew install sqlcipher)")
endif()
if(NOT KSTACK_HAS_PERSISTENCE)
  message(STATUS "Encrypted history unavailable; this build keeps session history only")
endif()
qt_standard_project_setup(REQUIRES 6.4)
