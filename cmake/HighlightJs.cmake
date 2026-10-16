## Author: Runor Ewhro
## Description: fetches highlight.js's browser build at a pinned, checksummed
##              release. The app runs it in Qt's JavaScript engine only to
##              settle close calls when naming the language of copied code.
include(FetchContent)

# 11.11.1, not 11.12.0: 11.12.0's automatic detection answers Dart for most
# code, which would make it useless as a tie-breaker
FetchContent_Declare(kstack_highlightjs
  URL https://registry.npmjs.org/@highlightjs/cdn-assets/-/cdn-assets-11.11.1.tgz
  URL_HASH SHA256=28a06bf6ebcc2218c6e842bdb4f1c8353508a8cada59e85110f91892f5726195
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  # a script bundle, not a CMake project
  SOURCE_SUBDIR no-cmake-build
)
FetchContent_MakeAvailable(kstack_highlightjs)

# the core build plus the languages with clues that it leaves out
set(KSTACK_HIGHLIGHTJS_FILES
  highlight.min.js
  languages/cmake.min.js
  languages/dart.min.js
  languages/dockerfile.min.js
  languages/dos.min.js
  languages/elixir.min.js
  languages/haskell.min.js
  languages/powershell.min.js
  languages/scala.min.js
  LICENSE
)

# adds the scripts and their license to a target's resources under
# :/highlightjs/
function(kstack_add_highlightjs target)
  set(files)
  foreach(file IN LISTS KSTACK_HIGHLIGHTJS_FILES)
    list(APPEND files ${kstack_highlightjs_SOURCE_DIR}/${file})
  endforeach()
  qt_add_resources(${target} "${target}_highlightjs"
    PREFIX /highlightjs
    BASE ${kstack_highlightjs_SOURCE_DIR}
    FILES ${files}
  )
endfunction()
