INCLUDE_DIRECTORIES(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIR}
  ${GLADEMM_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
)

SET(cppmod_coverartprefs_SOURCES
  prefs-coverart.hh
  prefs-coverart.cc
)

# FIXME: Are these flags included? -export-dynamic -fPIC -module
ADD_LIBRARY(cppmod-coverartprefs MODULE ${cppmod_coverartprefs_SOURCES})

TARGET_LINK_LIBRARIES(cppmod-coverartprefs
  ${GLADEMM_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${GTKMM_LIBRARIES}
)

INSTALL(TARGETS cppmod-coverartprefs LIBRARY DESTINATION ${PLUGIN_DIR}/cppmod)
