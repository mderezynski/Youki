INCLUDE_DIRECTORIES(
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR} # For generated headers
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include/mpx
  ${GLIB_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
  ${SEXYMM_INCLUDE_DIRS}
  ${SIGX_INCLUDE_DIRS}
  ${GIOMM_INCLUDE_DIRS}
  ${GLADEMM_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
  ${SEXYMM_INCLUDE_DIRS}
)

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/widget-marshalers.c ${CMAKE_CURRENT_BINARY_DIR}/widget-marshalers.h
  COMMAND ${GLIB_GENMARSHAL} --prefix=widget --body ${CMAKE_CURRENT_SOURCE_DIR}/widget-marshalers.list   > ${CMAKE_CURRENT_BINARY_DIR}/widget-marshalers.c
  COMMAND ${GLIB_GENMARSHAL} --prefix=widget --header ${CMAKE_CURRENT_SOURCE_DIR}/widget-marshalers.list > ${CMAKE_CURRENT_BINARY_DIR}/widget-marshalers.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/widget-marshalers.list
)

SET(mpx_widgets_SOURCES
  youki-status-icon.c
  youki-status-icon.h
  youki-tray-icon.c
  youki-tray-icon.h
  youki-clutter-widget.cc
  cairo-extensions.cc
  cell-renderer-album-data.cc
  cell-renderer-cairo-surface.cc
  cell-renderer-count.cc
  cell-renderer-expander.c
  cell-renderer-source.cc
  cell-renderer-vbox.cc
  dialog-simple-entry.cc
  rounded-layout.cc
  rounded-alignment.cc
  seekscale.cc
  task-dialog.cc
  timed-confirmation.cc
  tidy-actor.c
  tidy-actor.h
  tidy-debug.h
  tidy-marshal.c
  tidy-marshal.h
  tidy-private.h
  tidy-stylable.c
  tidy-stylable.h
  tidy-style.c
  tidy-style.h
  tidy-texture-reflection.c
  tidy-texture-reflection.h
  tidy-types.h
  ${CMAKE_CURRENT_BINARY_DIR}/widget-marshalers.c
  ${CMAKE_CURRENT_BINARY_DIR}/widget-marshalers.h
)

SET_SOURCE_FILES_PROPERTIES(widget-marshalers.c PROPERTIES GENERATED true)

ADD_LIBRARY(mpx-widgets SHARED ${mpx_widgets_SOURCES})

SET_SOURCE_FILES_PROPERTIES(
  widget-marshalers.c
  widget-marshalers.h
  PROPERTIES
  GENERATED yes
)

TARGET_LINK_LIBRARIES(mpx-widgets
  ${GLIBMM_LIBS}
  ${GTKMM_LIBS}
  ${GLADEMM_LIBS}
)

INSTALL(TARGETS mpx-widgets LIBRARY DESTINATION ${MPX_LIBDIR})
