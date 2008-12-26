message (STATUS "checking for HAL PropertySetIterator")

# FIXME: add proper link libraries
try_run (HAL_PSI_TEST_EXITCODE HAL_PSI_TEST_COMPILED
  ${CMAKE_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/cmake/hal-psi-test.cpp
)

if (HAL_PSI_TEST_EXITCODE EQUAL 0)
  message (STATUS "  PropertySetIterator found")
  set (HAL_PSI_FOUND "YES")
else (HAL_PSI_TEST_EXITCODE EQUAL 0)
  message (STATUS "  PropertySetIterator not found")
  set (HAL_PSI_FOUND "NO")
endif (HAL_PSI_TEST_EXITCODE EQUAL 0)

#AC_DEFINE(HAVE_HAL_NEWPSI,,[define when having newstyle HAL PSI])
