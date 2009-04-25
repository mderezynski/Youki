IF(HAVE_HAL)
  SET(youki_bin_ADD_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/libhal++)
  SET(youki_bin_ADD_LIBRARIES halcc)
ENDIF(HAVE_HAL)

INCLUDE_DIRECTORIES(
  ${ALSA_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR}
  ${CAIROMM_INCLUDE_DIRS}
  ${CDIO_INCLUDE_DIRS}
  ${CDPARANOIA_INCLUDE_DIRS}
  ${CLUTTER_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${GDL_INCLUDE_DIRS}
  ${GIOMM_INCLUDE_DIRS}
  ${GLADEMM_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${GTKGLEXT_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
  ${HAL_INCLUDE_DIRS}
  ${NM_INCLUDE_DIRS}
  ${LIBOFA_INCLUDE_DIRS}
  ${PYGTK_INCLUDE_DIRS}
  ${PYTHON_INCLUDE_PATH}
  ${RSVG_INCLUDE_DIRS}
  ${SEXYMM_INCLUDE_DIRS}
  ${SIGX_INCLUDE_DIRS}
  ${SOUP_INCLUDE_DIRS}
  ${SQLITE_INCLUDE_DIRS}
  ${STARTUP_NOTIFY_INCLUDE_DIRS}
  ${TAGLIB_INCLUDE_DIRS}
  ${X11_X11_INCLUDE_PATH}
  ${XML_INCLUDE_DIRS}
  ${XPROTO_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src # FIXME: Get rid of cross header dependencies between /src and /include
  ${youki_bin_add_INCLUDE_DIRS}
)

SET(youki_bin_SOURCES
  covers.cc
  covers-stores.cc
  dialog-about.hh
  dialog-about.cc
  dialog-equalizer.hh
  dialog-equalizer.cc
  glib-marshalers.c
  glib-marshalers.h
  youki-spectrum.hh
  youki-spectrum.cc
  jnethttpsrc.h
  jnethttpsrc.cc
  kobo-position.hh
  kobo-position.cc
  youki-controller.hh
  youki-controller.cc
  youki-controller-status-icon.hh
  youki-controller-status-icon.cc
  youki-notification.hh
  youki-notification.cc
  youki-simpleinfo.hh
  youki-simpleinfo.cc
  youki-togglebutton.hh
  youki-togglebutton.cc
  kobo-cover.hh
  kobo-cover.cc
  kobo-main.hh
  kobo-main.cc
  kobo-titleinfo.hh
  kobo-titleinfo.cc
  kobo-volume.hh
  kobo-volume.cc
  youki-theme-engine.hh
  youki-theme-engine.cc
  library.cc
  library.hh
  mpx-mlibman-dbus-proxy-actual.hh
  mpx-mlibman-dbus-proxy.hh
  mpx-app-dbus-adaptor.hh
  markov-analyzer-thread.cc
  markov-analyzer-thread.hh
  play.hh
  play.cc
  python.cc
  pysigc.hh
  plugin.cc
  plugin-loader-python.hh
  plugin-loader-python.cc
  plugin-types-python.cc
  plugin-types-python.hh
  plugin-loader-cpp.cc
  plugin-loader-cpp.hh
  plugin-types-cpp.hh
  plugin-loader.hh
  plugin-manager-gui.hh
  plugin-manager-gui.cc
  gtkmm.cc
  gtkmmmodule.cc
  gtkmmmodule.h
  preferences.cc
  preferences.hh
  splash-screen.hh
  splash-screen.cc
  stock.cc
  main.cc
)

# FIXME: Add definitions from: ${GLIB_CFLAGS} and ${GST_CFLAGS}

SET(youki_bin_COMPILE_DEFINITIONS __USE_POSIX199309)

IF(HAVE_LINUX)
  SET(youki_bin_COMPILE_DEFINITIONS "${youki_bin_COMPILE_DEFINITIONS} _XOPEN_SOURCE=600")
ENDIF(HAVE_LINUX)

ADD_EXECUTABLE(youki-bin ${youki_bin_SOURCES})

SET_TARGET_PROPERTIES(youki-bin
  PROPERTIES
  COMPILE_DEFINITIONS "${youki_bin_COMPILE_DEFINITIONS}"
  LINK_FLAGS "-Wl,--export-dynamic"
)

TARGET_LINK_LIBRARIES(youki-bin
  ${ALSA_LIBRARIES}
  ${Boost_REGEX_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_PYTHON_LIBRARY}
  ${CAIROMM_LIBRARIES}
  ${CLUTTER_LIBRARIES}
  ${CDIO_LIBRARIES}
  ${CDPARANOIA_LIBRARY}
  ${CURL_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${GDL_LIBRARIES}
  ${GIOMM_LIBRARIES}
  ${GLADEMM_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${GST_LIBRARIES}
  ${GTKGLEXT_LIBRARIES}
  ${GTKMM_LIBRARIES}
  ${HAL_LIBRARIES}
  ${NM_LIBRARIES}
  ${LIBOFA_LIBRARIES}
  ${PYGTK_LIBRARIES}
  ${PYTHON_LIBRARIES}
  ${RSVG_LIBRARIES}
  ${SEXYMM_LIBRARIES}
  ${SIGX_LIBRARIES}
  ${SM_LIBRARIES}
  ${SOUP_LIBRARIES}
  ${SQLITE_LIBRARIES}
  ${STARTUP_NOTIFY_LIBRARIES}
  ${TAGLIB_LIBRARIES}
  ${XML_LIBRARIES}
  ${X11_X11_LIB}
  jnetlib
  mconfig
  mpx-api
  mpx-widgets
  mpx-musicbrainz
  mpx-view-components
  mpx-json
  ${youki_bin_ADD_LIBRARIES}
)

ADD_CUSTOM_COMMAND(OUTPUT mpx-mlibman-dbus-proxy.hh
  COMMAND ${DBUSBINDINGTOOL} ARGS "${PROJECT_SOURCE_DIR}/dbus/dbus-obj-MLIBMAN.xml" "--proxy=${PROJECT_SOURCE_DIR}/src/mpx-mlibman-dbus-proxy.hh"
  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/dbus/dbus-obj-MLIBMAN.xml
)

ADD_CUSTOM_COMMAND(OUTPUT mpx-app-dbus-adaptor.hh
  COMMAND ${DBUSBINDINGTOOL} ARGS "${PROJECT_SOURCE_DIR}/dbus/dbus-obj-YOUKI.xml" "--adaptor=${PROJECT_SOURCE_DIR}/src/mpx-app-dbus-adaptor.hh"
  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/dbus/dbus-obj-YOUKI.xml
)

INSTALL(TARGETS youki-bin RUNTIME DESTINATION ${MPX_LIBEXECDIR})
