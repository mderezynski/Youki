INCLUDE_DIRECTORIES(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${TAGLIB_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
)

SET(mpx_xiph_reader_SOURCES
  reader.cc
  reader.hh
)

# FIXME: Are these flags included? -fPIC -module
ADD_LIBRARY(mpx_xiph_reader SHARED ${mpx_xiph_reader_SOURCES})

TARGET_LINK_LIBRARIES(mpx_xiph_reader
  ${TAGLIB_LIBRARIES}
  ${GLIBMM_LIBRARIES}
)

# FIXME: this looks fishy
#  noinst_LTLIBRARIES = libmpx_xiph_reader.la
#  libmpx_xiph_readerdir = ${libdir}
