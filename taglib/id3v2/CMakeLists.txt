INCLUDE_DIRECTORIES(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${TAGLIB_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
)

SET(mpx_id3v2_reader_SOURCES
  reader.cc
  reader.hh
)

# FIXME: Are these flags included? -fPIC -module
ADD_LIBRARY(mpx_id3v2_reader SHARED ${mpx_id3v2_reader_SOURCES})
TARGET_LINK_LIBRARIES(mpx_id3v2_reader
  ${TAGLIB_LIBRARIES}
  ${GLIBMM_LIBRARIES}
)

# FIXME: This looks fishy..
#
#  noinst_LTLIBRARIES = libmpx_id3v2_reader.la
#  libmpx_id3v2_readerdir = ${libdir}
