pkg_check_modules (CDIO libcdio_cdda>=0.70)

find_path (CDDA_INCLUDE_DIR
  NAMES cdda/cdda_interface.h
  HINTS $ENV{CDDADIR}
  PATH_SUFFIXES include
  PATHS /usr/local /usr /opt/local opt
)

find_library (CDDA_LIBRARY
  NAMES cdda_interface
  HINTS $ENV{CDDADIR}
  PATH_SUFFIXES lib64 lib
  PATHS /usr/local /usr /opt/local /opt
)

set (CDDA_FOUND "NO")
if (CDDA_INCLUDE_DIR AND CDDA_LIBRARY)
  set (CDDA_FOUND "YES")
endif (CDDA_INCLUDE_DIR AND CDDA_LIBRARY)
