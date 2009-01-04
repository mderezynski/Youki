set (LIBHALCC_SOVERSION 3.0.0)
set (LIBHALCC_VERSION 1.0)

set (LIBHALCC_SOURCES
  context.cc
  device.cc
  drive.cc
  volume.cc
  util.cc
)

set (LIBHALCC_COMMON_HEADERS
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

include_directories (${SIGC_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS} ${HAL_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})

set_source_files_properties (${LIBHALCC_SOURCES}
  PROPERTIES
  COMPILE_FLAGS "${SIGC_OTHER_CFLAGS} ${DBUS_OTHER_CFLAGS} ${HAL_OTHER_CFLAGS} ${HAL_OTHER_CFLAGS}"
)

if (HAVE_BMP)
  add_library (halcc STATIC ${LIBHALCC_SOURCES})
  set_target_properties (halcc PROPERTIES VERSION ${LIBHALCC_VERSION} SOVERSION ${LIBHALCC_SOVERSION})

else (HAVE_BMP)
  add_library (halcc SHARED ${LIBHALCC_SOURCES})
  set_target_properties (halcc PROPERTIES VERSION ${LIBHALCC_VERSION} SOVERSION ${LIBHALCC_SOVERSION})

  install (
    FILES ${LIBHALCC_COMMON_HEADERS}
    DESTINATION include/hal++-1.0/hal++
  )

  install (
    TARGETS halcc
    LIBRARY DESTINATION lib #${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION lib #${CMAKE_INSTALL_LIBDIR}
  )
endif (HAVE_BMP)

target_link_libraries (halcc ${SIGC_LIBRARIES} ${DBUS_LIBRARIES} ${HAL_LIBRARIES} ${GLIB_LIBRARIES})
