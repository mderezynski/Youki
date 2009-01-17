PKG_CHECK_MODULES(HAL hal>=0.5.7.1 hal-storage>=0.5.7.1)

IF(HAL_FOUND)
  PKG_CHECK_MODULES(HAL_058 hal>=0.5.8.1 hal-storage>=0.5.8.1)

  MESSAGE(STATUS "checking for HAL PropertySetIterator")

  TRY_RUN(HAL_PSI_TEST_EXITCODE HAL_PSI_TEST_COMPILED
    ${CMAKE_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/cmake/hal-psi-test.cpp
    CMAKE_FLAGS
      -DINCLUDE_DIRECTORIES:STRING=${HAL_INCLUDE_DIRS}
      -DLINK_LIBRARIES:STRING=${HAL_LIBRARIES}
  )

  IF(HAL_PSI_TEST_EXITCODE EQUAL 0)
    MESSAGE(STATUS "  PropertySetIterator found")
    SET(HAL_PSI_FOUND "YES")
  ELSE(HAL_PSI_TEST_EXITCODE EQUAL 0)
    MESSAGE(STATUS "  PropertySetIterator not found")
    SET(HAL_PSI_FOUND "NO")
  ENDIF(HAL_PSI_TEST_EXITCODE EQUAL 0)

ENDIF(HAL_FOUND)
