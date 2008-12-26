#pkg_check_modules (SM sm)

if (NOT SM_FOUND)
  find_path (SM_INCLUDE_DIR
    NAMES SM
    HINTS $ENV{SMDIR}
    PATHS /usr/local /usr /opt/local /opt
    PATH_SUFFIXES include/X11 include X11R6/include
  )

  find_library (SM_LIBRARY
    NAMES SM
    HINTS $ENV{SMDIR}
    PATHS /usr/local/X11R6 /usr/X11R6 /opt/local/X11R6 /opt/X11R6
    PATH_SUFFIXES lib64 lib
  )

  find_library (ICE_LIBRARY
    NAMES ICE
    HINTS $ENV{SMDIR}
    PATHS /usr/local/X11R6 /usr/X11R6 /opt/local/X11R6 /opt/X11R6
    PATH_SUFFIXES lib64 lib
  )

  if (SM_INCLUDE_DIR AND SM_LIBRARY AND ICE_LIBRARY)
    set (SM_FOUND "YES")
    set (SM_LIBRARIES ${SM_LIBRARY} ${ICE_LIBRARY})
  endif (SM_INCLUDE_DIR AND SM_LIBRARY AND ICE_LIBRARY)

endif (NOT SM_FOUND)
