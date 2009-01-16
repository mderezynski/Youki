SET(mpx_musicbrainz_SOURCES
  mb-libxml2-sax-release.cc
  mb-libxml2-sax-release-list.cc
  mb-libxml2-sax-tracks.cc
  mb-utils.cc
  mbxml-v2.cc
)

# FIXME: Include these files into distribution
#  mb-libxml2-sax-release.hh
#  mb-libxml2-sax-release-list.hh
#  mb-libxml2-sax-tracks.hh
#  mb-utils.hh
#  mbxml-v2.hh
#  mbxml-types-v2.hh

INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/mpx
  ${SOUP_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
  ${GLIBMM_INCLUDE_DIRS}
  ${XML_INCLUDE_DIRS}
)

ADD_LIBRARY(mpx_musicbrainz SHARED ${mpx_musicbrainz_SOURCES})

TARGET_LINK_LIBRARIES(mpx_musicbrainz
  ${Boost_FILESYSTEM_LIBRARY}
  ${SOUP_LIBRARIES}
)
