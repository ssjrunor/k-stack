add_library(kstack_storage STATIC
  src/Database.cpp
  src/EntryRepository.cpp
  src/SettingsRepository.cpp
  src/EncryptionProvider.cpp
)
target_include_directories(kstack_storage PUBLIC include)
target_link_libraries(kstack_storage PUBLIC kstack_core SQLite3::SQLite3)
kstack_enable_warnings(kstack_storage)
