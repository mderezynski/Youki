pkg_check_modules (SM sm)

if (NOT SM_FOUND)
  find_package (X11)

  message (STATUS "checking for X11 session management library (SMlib)")

  if (X11_ICE_FOUND)
    find_file (SM_HEADER
      NAME SM.h
      PATHS ${X11_INCLUDE_DIR}
      PATH_SUFFIXES SM
    )

    find_library (SM_LIBRARY
      NAME SM
      PATHS /usr/local/X11R6 /usr/X11R6
      PATH_SUFFIXES lib lib64
    )

    if (NOT ${SM_HEADER}  MATCHES "NOTFOUND" AND
        NOT ${SM_LIBRARY} MATCHES "NOTFOUND")

      set (SM_FOUND "YES")
      string (REPLACE "SM/SM.h" "" SM_INCLUDE_PATH ${SM_HEADER})
      set (SM_LIBRARIES "${SM_LIBRARY} ${X11_ICE_LIB}")

    endif (NOT ${SM_HEADER}  MATCHES "NOTFOUND" AND
           NOT ${SM_LIBRARY} MATCHES "NOTFOUND")

  endif (X11_ICE_FOUND)

  if (SM_FOUND)
    message (STATUS "  SMlib found")
  else (SM_FOUND)
    message (STATUS "  SMlib not found")
  endif (SM_FOUND)

endif (NOT SM_FOUND)
