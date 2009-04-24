INCLUDE_DIRECTORIES(cppmod_audioqualityprefs
  ${ALSA_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${BOOST_CPPFLAGS}
  ${DBUS_INCLUDE_DIRS}
  ${GLADEMM_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
  ${X11_X11_INCLUDE_PATH}
  ${XPROTO_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

SET(cppmod_audioqualityprefs
  prefs-audio-quality.hh
  prefs-audio-quality.cc
)

# FIXME: All these flags included? -export-dynamic -fPIC -module
ADD_LIBRARY(cppmod-audioqualityprefs MODULE ${cppmod_audioqualityprefs})

TARGET_LINK_LIBRARIES(cppmod-audioqualityprefs
  ${ALSA_LIBRARIES}
  ${GST_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${GLADEMM_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${GTKMM_LIBRARIES}
  ${X11_X11_LIB}
)

# FIXME: Install plugin into ${PLUGIN_DIR}/cppmod
