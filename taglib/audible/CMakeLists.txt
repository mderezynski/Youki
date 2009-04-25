INCLUDE_DIRECTORIES(
  ${TAGLIB_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

SET(taglib_plugin_audible_SOURCES 
  plugin.cc
  audibleproperties.cpp
  audibleproperties.h
  audibletag.cpp
  audibletag.h
  taglib_audiblefile.cpp
  taglib_audiblefile.h
  resolver.cpp
  resolver.h
)

# FIXME: Are these flags included? -fPIC -module
ADD_LIBRARY(taglib_plugin_audible ${taglib_plugin_audible_SOURCES})

TARGET_LINK_LIBRARIES(taglib_plugin_audible
  ${TAGLIB_LIBARIES}
  ${GLIBMM_LIBARIES}
)

INSTALL(TARGETS taglib_plugin_audible LIBRARY DESTIANTION  ${PLUGIN_DIR}/taglib)
