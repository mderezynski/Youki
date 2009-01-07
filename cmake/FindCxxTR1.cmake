message (STATUS "checking for C++ TR1 extensions")

try_run (TR1_TEST_EXITCODE TR1_TEST_COMPILED
  ${CMAKE_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/cmake/tr1-test.cpp
)

if (TR1_TEST_EXITCODE EQUAL 0)
  message (STATUS "  C++ TR1 extensions found")
  set (TR1_FOUND "YES")
else (TR1_TEST_EXITCODE EQUAL 0)
  message (STATUS "  C++ TR1 extensions not found")
  set (TR1_FOUND "NO")
endif (TR1_TEST_EXITCODE EQUAL 0)
