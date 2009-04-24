INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${TAGLIB_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
)

SET(mpx_common_reader_SOURCES
  common.cc 
)

ADD_LIBRARY(mpx_common_reader MODULE ${mpx_common_reader_SOURCES})

#libmpx_common_reader_la_LDFLAGS = -fPIC -module

TARGET_LINK_LIBRARIES(mpx_common_reader
  ${TAGLIB_LIBRARIES}
  ${GLIBMM_LIBRARIES}
)

# FIXME: Include headers in distribution
#  common.hh

#libmpx_common_readerdir = $(libdir)
