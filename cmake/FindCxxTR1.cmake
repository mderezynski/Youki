TRY_RUN(CxxTR1_TEST_EXITCODE CxxTR1_TEST_COMPILED
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/cmake/tr1-test.cpp
)

IF(CxxTR1_TEST_EXITCODE EQUAL 0)
  SET(CxxTR1_FOUND yes)
  INCLUDE(FindPackageMessage)
  FIND_PACKAGE_MESSAGE(CxxTR1 "Found C++ TR1 extensions" "${CxxTR1_TEST_EXITCODE}")
ELSE(CxxTR1_TEST_EXITCODE EQUAL 0)
  SET(CxxTR1_FOUND no)
  IF(CxxTR1_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "C++ TR1 extensions not found")
  ELSE(CxxTR1_FIND_REQUIRED)
    IF(NOT CxxTR1_FIND_QUIETLY)
      MESSAGE(STATUS "C++ TR1 extensions not found")
    ENDIF(NOT CxxTR1_FIND_QUIETLY)
  ENDIF(CxxTR1_FIND_REQUIRED)
ENDIF(CxxTR1_TEST_EXITCODE EQUAL 0)
