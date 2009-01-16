# FIXME: Centralise this
SET_PROPERTY(DIRECTORY PROPERTY COMPILE_DEFINITIONS HAVE_CONFIG_H)

SET(mconfig_SOURCES
  subscriber.cpp
  key.cpp
  mcs.cpp
  mcs-gtk-bind.cpp
)

# FIXME: Include headers in distribution
#  mcs.h
#  types.h
#  subscriber.h
#  key.h
#  base.h
#  gtk-bind.h	  

INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${XML_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
)

ADD_LIBRARY(mconfig SHARED ${mconfig_SOURCES})

TARGET_LINK_LIBRARIES(mconfig
  ${GLIB_LIBS}
  ${GLIBMM_LIBS}
  ${GTKMM_LIBS}
  ${XML_LIBS}
)

#libmconfig_la_LDFLAGS  = -export-dynamic -fPIC -no-undefined #-export-symbols $(top_srcdir)/libmconfig/libmconfig.sym
