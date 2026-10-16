## Author: Runor Ewhro
## Description: adapts the pinned Bergamot sources to current compilers and
##              CMake; runs in the fetched source directory, and running it
##              again leaves already patched files alone.
function(kstack_patch file before after)
  file(READ "${file}" content)
  string(FIND "${content}" "${after}" applied)
  if(NOT applied EQUAL -1)
    return()
  endif()
  string(FIND "${content}" "${before}" found)
  if(found EQUAL -1)
    message(FATAL_ERROR "Bergamot patch no longer matches ${file}")
  endif()
  string(REPLACE "${before}" "${after}" content "${content}")
  file(WRITE "${file}" "${content}")
endfunction()

# Clang 18+ reports variable-length arrays as an extension, and Marian builds
# with -Werror
kstack_patch(3rd_party/marian-dev/CMakeLists.txt
  "set(CLANG_IGNORE_UNUSED_PRIVATE_FIELD -Wno-unused-private-field;)"
  "set(CLANG_IGNORE_UNUSED_PRIVATE_FIELD -Wno-unused-private-field; -Wno-vla-cxx-extension;)")
# the bundled zlib defines fdopen away on macOS, which breaks the SDK's stdio.h
kstack_patch(3rd_party/marian-dev/src/3rd_party/zlib/zutil.h
  "#      ifndef fdopen\n"
  "#      if !defined(fdopen) && !defined(__APPLE__)\n")
# current Clang rejects a constexpr cast of -1 to an enum without a fixed type
kstack_patch(3rd_party/marian-dev/src/3rd_party/sentencepiece/src/trainer_interface.cc
  "  constexpr unicode_script::ScriptType kAnyType ="
  "  const unicode_script::ScriptType kAnyType =")
# PCRE2 is configured as a separate project, which CMake 4 refuses without a
# policy floor
kstack_patch(3rd_party/ssplit-cpp/cmake/FindPCRE2.cmake
  "CONFIGURE_COMMAND \${CMAKE_COMMAND} \${PCRE2_SRC_DIR} \${PCRE2_CONFIGURE_OPTIONS}\n"
  "CONFIGURE_COMMAND \${CMAKE_COMMAND} \${PCRE2_SRC_DIR} \${PCRE2_CONFIGURE_OPTIONS} -DCMAKE_POLICY_VERSION_MINIMUM=3.5\n")
