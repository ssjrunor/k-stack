## Author: Runor Ewhro
## Description: fetches the code language detector (codelang) at a pinned
##              commit; as a subproject it builds only the library.
include(FetchContent)

FetchContent_Declare(kstack_codelang
  GIT_REPOSITORY https://github.com/ssjrunor/code-language-detector.git
  GIT_TAG 306ae96e53283bae126ccdb795a8cdce591b8098
)
FetchContent_MakeAvailable(kstack_codelang)
