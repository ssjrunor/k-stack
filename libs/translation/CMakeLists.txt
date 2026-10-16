add_library(kstack_translation STATIC
  src/LanguageTag.cpp
  src/Routes.cpp
)
target_include_directories(kstack_translation PUBLIC include)
kstack_enable_warnings(kstack_translation)

if(KSTACK_ENABLE_TRANSLATION)
  target_sources(kstack_translation PRIVATE src/Cld2LanguageDetector.cpp)
  target_link_libraries(kstack_translation PRIVATE kstack_cld2)
  # Marian's headers need C++17, so the Bergamot adapter builds on its own;
  # it only sees the library's plain C++ interface
  add_library(kstack_translation_bergamot STATIC src/BergamotTranslator.cpp)
  set_target_properties(kstack_translation_bergamot PROPERTIES CXX_STANDARD 17)
  target_include_directories(kstack_translation_bergamot PRIVATE include)
  target_include_directories(kstack_translation_bergamot SYSTEM PRIVATE
    $<TARGET_PROPERTY:bergamot-translator,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:marian,INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(kstack_translation_bergamot PRIVATE bergamot-translator)
  target_link_libraries(kstack_translation PUBLIC kstack_translation_bergamot)
  # the adapter calls findRoute from the main library
  target_link_libraries(kstack_translation_bergamot INTERFACE kstack_translation)
else()
  target_sources(kstack_translation PRIVATE src/TranslationUnavailable.cpp)
endif()
