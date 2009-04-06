libcppmod_lastfmscrobbler_LTLIBRARIES = libcppmod-lastfmscrobbler.la

libcppmod_lastfmscrobbler_la_LDFLAGS = -export-dynamic -fPIC -module

libcppmod_lastfmscrobbler_la_LIBADD =           \
                                                \
        $(BOOST_LDFLAGS)                        \
        $(CURL_LIBS)                            \
        $(DBUS_LIBS)                            \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GTKMM_LIBS)                           

libcppmod_lastfmscrobbler_la_CXXFLAGS =         \
                                                \
        $(BOOST_CPPFLAGS)                       \
        $(CURL_CFLAGS)                          \
        $(DBUS_CFLAGS)                          \
        $(GLADEMM_CFLAGS)                       \
        $(GLIBMM_CFLAGS)                        \
        $(GTKMM_CFLAGS)                         \
        -I$(top_srcdir)                         \
        -I$(top_srcdir)/include                 \
        -DPLUGIN_BUILD=1                        \
        -DENABLE_DEBUG=1            


libcppmod_lastfmscrobbler_la_SOURCES =          \
                                                \
        cppmod-scrobbler.hh                     \
        cppmod-scrobbler.cc                     \
                                                \
        condition.cpp                           \
        condition.h                             \
        lastfmclient.cpp                        \
        lastfmclient.h                          \
        lastfmexceptions.h                      \
        lastfmscrobbler.cpp                     \
        lastfmscrobbler.h                       \
        lastfmtypes.h                           \
        log.cpp                                 \
        log.h                                   \
        md5.c                                   \
        md5.h                                   \
        mutex.cpp                               \
        mutex.h                                 \
        nowplayinginfo.cpp                      \
        nowplayinginfo.h                        \
        scopedlock.cpp                          \
        scopedlock.h                            \
        stringoperations.cpp                    \
        stringoperations.h                      \
        submissioninfocollection.cpp            \
        submissioninfocollection.h              \
        submissioninfo.cpp                      \
        submissioninfo.h                        \
        thread.cpp                              \
        thread.h                                \
        urlclient.cpp                           \
        urlclient.h

libcppmod_lastfmscrobblerdir = $(PLUGIN_DIR)/cppmod
