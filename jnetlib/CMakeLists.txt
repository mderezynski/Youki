SET(jnetlib_SOURCES
  asyncdns.cpp
  connection.cpp
  httpget.cpp
  httppost.cpp
  listen.cpp
  util.cpp
)

ADD_DEFINITIONS(-DTHREAD_SAFE -D_THREAD_SAFE -D_REENTRANT)

INCLUDE_DIRECTORIES(
  ${GLIB_INCLUDE_DIRS}
  ${SIGC_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
)

ADD_LIBRARY(jnetlib STATIC ${jnetlib_SOURCES})

TARGET_LINK_LIBRARIES(jnetlib
  ${GLIB_LIBS}
  ${GLIBMM_LIBS}
  ${SIGC_LIBS}
  ${GTK_LIBS}
)

#libjnetlib_la_LDFLAGS = -pthread 

# FIXME: Include headers in distribution
#  exception.hh
#  asyncdns.h
#  connection.h
#  httpget.h
#  httppost.h
#  httpserv.h
#  jnetlib.h
#  listen.h
#  netinc.h
#  sslconnection.h
#  ssllisten.h
#  udpconnection.h
#  util.h
