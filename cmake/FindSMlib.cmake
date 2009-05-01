#PKG_CHECK_MODULES(SM sm)

IF(NOT SM_FOUND)
  FIND_PATH(SM_INCLUDE_DIR SM
    HINTS $ENV{SMDIR}
  )
  MARK_AS_ADVANCED(SM_INCLUDE_DIR)

  FIND_LIBRARY(SM_LIBRARY SM
    HINTS $ENV{SMDIR}
  )
  MARK_AS_ADVANCED(SM_LIBRARY)

  FIND_LIBRARY(ICE_LIBRARY ICE
    HINTS $ENV{SMDIR}
  )
  MARK_AS_ADVANCED(ICE_LIBRARY)

  FIND_PACKAGE_HANDLE_STANDARD_ARGS(SM DEFAULT_MSG SM_INCLUDE_DIR SM_LIBRARY ICE_LIBRARY)

  IF(SM_FOUND)
    SET(SM_LIBRARIES ${SM_LIBRARY} ${ICE_LIBRARY})
  ENDIF(SM_FOUND)

ENDIF(NOT SM_FOUND)
