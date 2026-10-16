## Author: Runor Ewhro
## Description: fetches the on-device translation engine (Bergamot) and the
##              language detector (CLD2) at pinned commits; turning the option
##              off builds the app without translation.
option(KSTACK_ENABLE_TRANSLATION "Translate captured text on the device" ON)
if(NOT KSTACK_ENABLE_TRANSLATION)
  return()
endif()

include(FetchContent)

FetchContent_Declare(kstack_cld2
  GIT_REPOSITORY https://github.com/CLD2Owners/cld2.git
  GIT_TAG b56fa78a2fe44ac2851bae5bf4f4693a0644da7b
  # CLD2 has no CMake build; a missing subdirectory makes this fetch only
  SOURCE_SUBDIR no-cmake-build
)
FetchContent_Declare(kstack_bergamot
  GIT_REPOSITORY https://github.com/browsermt/bergamot-translator.git
  GIT_TAG 9271618ebbdc5d21ac4dc4df9e72beb7ce644774
  GIT_SUBMODULES_RECURSE ON
  PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/patches/bergamot.cmake
)

# CLD2 ships shell scripts rather than a build; these are the sources of its
# standard (non-"full") table set, about 80 languages
FetchContent_MakeAvailable(kstack_cld2)
set(KSTACK_CLD2_DIR ${kstack_cld2_SOURCE_DIR}/internal)
add_library(kstack_cld2 STATIC
  ${KSTACK_CLD2_DIR}/cldutil.cc ${KSTACK_CLD2_DIR}/cldutil_shared.cc
  ${KSTACK_CLD2_DIR}/compact_lang_det.cc ${KSTACK_CLD2_DIR}/compact_lang_det_hint_code.cc
  ${KSTACK_CLD2_DIR}/compact_lang_det_impl.cc ${KSTACK_CLD2_DIR}/debug.cc
  ${KSTACK_CLD2_DIR}/fixunicodevalue.cc ${KSTACK_CLD2_DIR}/generated_entities.cc
  ${KSTACK_CLD2_DIR}/generated_language.cc ${KSTACK_CLD2_DIR}/generated_ulscript.cc
  ${KSTACK_CLD2_DIR}/getonescriptspan.cc ${KSTACK_CLD2_DIR}/lang_script.cc
  ${KSTACK_CLD2_DIR}/offsetmap.cc ${KSTACK_CLD2_DIR}/scoreonescriptspan.cc
  ${KSTACK_CLD2_DIR}/tote.cc ${KSTACK_CLD2_DIR}/utf8statetable.cc
  ${KSTACK_CLD2_DIR}/cld_generated_cjk_uni_prop_80.cc
  ${KSTACK_CLD2_DIR}/cld2_generated_cjk_compatible.cc
  ${KSTACK_CLD2_DIR}/cld_generated_cjk_delta_bi_4.cc
  ${KSTACK_CLD2_DIR}/generated_distinct_bi_0.cc
  ${KSTACK_CLD2_DIR}/cld2_generated_quadchrome_2.cc
  ${KSTACK_CLD2_DIR}/cld2_generated_deltaoctachrome.cc
  ${KSTACK_CLD2_DIR}/cld2_generated_distinctoctachrome.cc
  ${KSTACK_CLD2_DIR}/cld_generated_score_quad_octa_2.cc
)
set_target_properties(kstack_cld2 PROPERTIES CXX_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
# its generated tables predate C++11's narrowing rules
target_compile_options(kstack_cld2 PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w -Wno-narrowing>)
target_include_directories(kstack_cld2 SYSTEM PUBLIC
  ${kstack_cld2_SOURCE_DIR}/public ${KSTACK_CLD2_DIR})

# Bergamot is added in a function so its settings stay in its own scope; it
# always builds optimised, since an unoptimised build translates too slowly
# to use even during development
function(kstack_add_bergamot)
  set(CMAKE_BUILD_TYPE Release)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  set(COMPILE_WASM OFF CACHE BOOL "" FORCE)
  set(USE_WASM_COMPATIBLE_SOURCE OFF CACHE BOOL "" FORCE)
  set(COMPILE_LIBRARY_ONLY ON CACHE BOOL "" FORCE)
  set(COMPILE_TESTS OFF CACHE BOOL "" FORCE)
  set(COMPILE_UNIT_TESTS OFF CACHE BOOL "" FORCE)
  set(COMPILE_PYTHON OFF CACHE BOOL "" FORCE)
  set(USE_MKL OFF CACHE BOOL "" FORCE)
  set(SSPLIT_USE_INTERNAL_PCRE2 ON CACHE BOOL "" FORCE)
  # FetchContent already checked out the submodules and patched them
  set(GIT_SUBMODULE OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(kstack_bergamot)
  # its command-line app is not needed
  set_target_properties(bergamot PROPERTIES EXCLUDE_FROM_ALL ON)
endfunction()
kstack_add_bergamot()
