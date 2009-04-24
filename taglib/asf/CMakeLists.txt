INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/taglib
  ${TAGLIB_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
)
  
SET(taglib_plugin_asf_SOURCES
  plugin.cc
  resolver.cpp
  asfattribute.cpp
  asffile.cpp
  asfproperties.cpp
  asftag.cpp
)

ADD_LIBRARY(taglib_plugin_asf MODULE ${taglib_plugin_asf_SOURCES})

#libtaglib_plugin_asf_la_LDFLAGS = -fPIC -module

TARGET_LINK_LIBRARIES(taglib_plugin_asf
  ${TAGLIB_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${PROJECT_BINARY_DIR}/taglib/common/libmpx_common_reader.so
)

# FIXME: Include headers in distribution
#  resolver.h
#  asfattribute.h
#  asffile.h
#  asfproperties.h
#  asftag.h

# FIXME: Install plugin in right directory
#  $(PLUGIN_DIR)/taglib
