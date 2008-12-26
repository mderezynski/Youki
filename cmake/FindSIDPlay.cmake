find_path (SIDPLAY_INCLUDE_DIR
  NAMES sidplay
  HINTS $ENV{SIDPLAYDIR}
  PATH_SUFFIXES include
  PATHS /usr/local /usr /opt/local opt
)
 
find_library (SIDPLAY_LIBRARY
  NAMES sidplay
  HINTS $ENV{SIDPLAYDIR}
  PATH_SUFFIXES lib64 lib
  PATHS /usr/local /usr /opt/local opt
)

set (SIDPLAY_FOUND "NO")
if (SIDPLAY_LIBRARY AND SIDPLAY_INCLUDE_DIR)
  set (SIDPLAY_FOUND "YES")
endif (SIDPLAY_LIBRARY AND SIDPLAY_INCLUDE_DIR)
