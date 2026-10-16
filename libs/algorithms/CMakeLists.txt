add_library(kstack_algorithms STATIC
  src/ContentHasher.cpp
  src/DuplicateDetector.cpp
  src/SearchRanker.cpp
  src/TextNormalizer.cpp
)
target_include_directories(kstack_algorithms PUBLIC include)
kstack_enable_warnings(kstack_algorithms)
