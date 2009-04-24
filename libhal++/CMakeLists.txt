INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${SIGC_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${HAL_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
)

SET(LIBHALCC_SOVERSION 3.0.0)
SET(LIBHALCC_VERSION 1.0)

SET(LIBHALCC_SOURCES
  context.cc
  device.cc
  drive.cc
  volume.cc
  util.cc
)

SET(LIBHALCC_COMMON_HEADERS
  context.hh
  device.hh
  exception.hh
  refptr.hh
  types.hh
  storage.hh
  drive.hh
  volume.hh
  hcc-macros.hh
  util.hh
  hal++.hh
)

# FIXME: Have we included all of these?
#   -export-dynamic -fPIC -version-info $(LIBVERSION) -release $(RELEASE)

ADD_LIBRARY(halcc SHARED ${LIBHALCC_SOURCES})

SET_TARGET_PROPERTIES(halcc
  PROPERTIES
  VERSION ${LIBHALCC_VERSION}
  SOVERSION ${LIBHALCC_SOVERSION}
)

TARGET_LINK_LIBRARIES(halcc ${SIGC_LIBRARIES} ${DBUS_LIBRARIES} ${HAL_LIBRARIES} ${GLIB_LIBRARIES})

INSTALL(TARGETS halcc LIBRARY DESTINATION ${MPX_LIBDIR})

INSTALL(
  FILES ${LIBHALCC_COMMON_HEADERS}
  DESTINATION ${MPX_INCLUDEDIR}/hal++-1.0/hal++
)
