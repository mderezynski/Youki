INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIR}
  ${DBUS_INCLUDE_DIRS}
  ${GLADEMM_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
  ${X11_X11_INCLUDE_PATH}
  ${XPROTO_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

SET(cppmod_mmkeys_SOURCES
  mmkeys.cc
)

ADD_LIBRARY(cppmod_mmkeys MODULE ${cppmod_mmkeys_SOURCES})

TARGET_LINK_LIBRARIES(cppmod_mmkeys
  ${DBUS_LIBRARIES}
  ${GLADEMM_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${GTKMM_LIBRARIES}
  ${X11_X11_LIB}
)

# FIXME: Are all these flags included?
#   libcppmod_mmkeys_la_LDFLAGS = -export-dynamic -fPIC -module

# FIXME: Include this in source distribution
#   mmkeys.hh

# FIXME: Install plugin in ${PLUGIN_DIR}/cppmod
