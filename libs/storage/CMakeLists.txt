add_library(kstack_storage STATIC
  src/Database.cpp
  src/EntryRepository.cpp
  src/SettingsRepository.cpp
)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/migrations/001_initial.sql" KSTACK_INITIAL_MIGRATION)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/migrations/002_entry_metadata.sql" KSTACK_ENTRY_METADATA_MIGRATION)
configure_file(migrations/MigrationSql.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/MigrationSql.hpp" @ONLY)
target_include_directories(kstack_storage PUBLIC include
  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(kstack_storage PRIVATE SQLITE_HAS_CODEC=1)
target_link_libraries(kstack_storage PUBLIC kstack_core SQLCipher::SQLCipher)
kstack_enable_warnings(kstack_storage)
