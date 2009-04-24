SET(mpx_widgets_SOURCES
  youki-status-icon.c
  youki-status-icon.h
  youki-tray-icon.c
  youki-tray-icon.h
  cairo-extensions.cc
  cell-renderer-album-data.cc
  cell-renderer-cairo-surface.cc
  cell-renderer-count.cc
  cell-renderer-expander.c
  cell-renderer-source.cc
  cell-renderer-vbox.cc
  dialog-simple-entry.cc
  rounded-layout.cc
  task-dialog.cc
  timed-confirmation.cc
  widget-marshalers.c
  widget-marshalers.h
)

SET_SOURCE_FILES_PROPERTIES(widget-marshalers.c PROPERTIES GENERATED true)

INCLUDE_DIRECTORIES(
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

ADD_LIBRARY(mpx-widgets SHARED ${mpx_widgets_SOURCES})

TARGET_LINK_LIBRARIES(mpx-widgets
  ${GLIBMM_LIBS}
  ${GTKMM_LIBS}
  ${GLADEMM_LIBS}
)

ADD_CUSTOM_COMMAND(
  OUTPUT widget-marshalers.c widget-marshalers.h
  COMMAND ${GLIB_GENMARSHAL} --prefix=widget --body widget-marshalers.list   > widget-marshalers.c
  COMMAND ${GLIB_GENMARSHAL} --prefix=widget --header widget-marshalers.list > widget-marshalers.h
  DEPENDS widget-marshalers.list
)

INSTALL(TARGETS mpx-widgets LIBRARY DESTINATION ${MPX_LIBDIR})
