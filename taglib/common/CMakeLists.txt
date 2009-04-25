INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${TAGLIB_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
)

SET(mpx_common_reader_SOURCES
  common.cc 
  common.hh
)

# FIXME: Are these flags included? -fPIC -module
ADD_LIBRARY(mpx_common_reader SHARED ${mpx_common_reader_SOURCES})

TARGET_LINK_LIBRARIES(mpx_common_reader
  ${TAGLIB_LIBRARIES}
  ${GLIBMM_LIBRARIES}
)

INSTALL(TARGETS mpx_common_reader LIBRARY DESTINATION ${MPX_LIBDIR})
