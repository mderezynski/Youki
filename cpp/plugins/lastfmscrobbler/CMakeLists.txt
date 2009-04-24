ADD_DEFINITIONS(-DENABLE_DEBUG=1)

SET(cppmod_lastfmscrobbler_SOURCES
  cppmod-scrobbler.cc
  condition.cpp
  lastfmclient.cpp
  lastfmscrobbler.cpp
  log.cpp
  md5.c
  mutex.cpp
  nowplayinginfo.cpp
  scopedlock.cpp
  stringoperations.cpp
  submissioninfocollection.cpp
  submissioninfo.cpp
  thread.cpp
  urlclient.cpp
)

INCLUDE_DIRECTORIES(
  ${Boost_INCLUDE_DIR}
  ${CURL_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${GLADEMM_INCLUDE_DIRS}
  ${GLIBMM_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

# FIXME: Are all these flags included? -dynamic -fPIC -module
ADD_LIBRARY(cppmod-lastfmscrobbler MODULE ${cppmod_lastfmscrobbler_SOURCES})

TARGET_LINK_LIBRARIES(cppmod-lastfmscrobbler
  ${CURL_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${GLADEMM_LIBRARIES}
  ${GLIBMM_LIBRARIES}
  ${GTKMM_LIBRARIES}
)

# FIXME: Include these into source distribution
#  condition.h
#  lastfmclient.h
#  lastfmexceptions.h
#  lastfmscrobbler.h
#  lastfmtypes.h
#  log.h
#  md5.h
#  mutex.h
#  nowplayinginfo.h
#  scopedlock.h
#  cppmod-scrobbler.hh
#  i-log.hh
#  urlclient.h
#  stringoperations.h
#  submissioninfocollection.h
#  submissioninfo.h
#  thread.h

# FIXME: Install plugin into ${PLUGIN_DIR}/cppmod
