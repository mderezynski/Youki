SET(mpx_parsers_SOURCES
  mb-artist-basic-1.0.cxx
  xsd-artist-similar-2.0.cxx
  xsd-artist-info-2.0.cxx
  xsd-topalbums-2.0.cxx
  xspf.cxx
)

# FIXME: Include these files into distribution
#  mb-artist-basic-1.0.hxx
#  xsd-artist-similar-2.0.hxx
#  xsd-artist-info-2.0.hxx
#  xsd-topalbums-2.0.hxx
#  xspf.hxx

INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${XML_INCLUDE_DIRS}
  ${XERCESC_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(mpx_parsers
  ${XERCESC_LIBRARIES}
)

ADD_LIBRARY(mpx_parsers STATIC ${mpx_parsers_SOURCES})
