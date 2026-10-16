# Packaging is deliberately opt-in until signed installers are defined.
option(KSTACK_ENABLE_PACKAGING "Enable CPack archive generation" OFF)
if(KSTACK_ENABLE_PACKAGING)
  set(CPACK_PACKAGE_NAME "k-stack")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  include(CPack)
endif()
