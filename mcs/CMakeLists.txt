INCLUDE_DIRECTORIES(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${XML_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
)

SET(mconfig_SOURCES
  base.h
  gtk-bind.h	  
  key.h
  key.cpp
  mcs.cpp
  mcs.h
  mcs-gtk-bind.cpp
  subscriber.cpp
  subscriber.h
  types.h
)

# FIXME: Have we included these?
#   -export-dynamic -fPIC -no-undefined -export-symbols $(top_srcdir)/libmconfig/libmconfig.sym

ADD_LIBRARY(mconfig STATIC ${mconfig_SOURCES})

TARGET_LINK_LIBRARIES(mconfig
  ${GLIB_LIBS}
  ${GLIBMM_LIBS}
  ${GTKMM_LIBS}
  ${XML_LIBS}
)
