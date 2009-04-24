INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${XML_INCLUDE_DIRS}
  ${XERCESC_INCLUDE_DIRS}
)

SET(mpx_parsers_SOURCES
  mb-artist-basic-1.0.cxx
  mb-artist-basic-1.0.hxx
  xsd-artist-similar-2.0.cxx
  xsd-artist-similar-2.0.hxx
  xsd-artist-info-2.0.cxx
  xsd-artist-info-2.0.hxx
  xsd-topalbums-2.0.cxx
  xsd-topalbums-2.0.hxx
  xspf.cxx
  xspf.hxx
)

ADD_LIBRARY(mpx-parsers SHARED ${mpx_parsers_SOURCES})

TARGET_LINK_LIBRARIES(mpx-parsers
  ${XERCESC_LIBRARIES}
)

INSTALL(TARGETS mpx-parsers LIBRARY DESTINATION ${MPX_LIBDIR})
