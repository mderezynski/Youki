INCLUDES = $(TAGLIB_CFLAGS) $(SID_INCLUDES) -I$(top_srcdir)
libtaglib_plugin_sid_LTLIBRARIES = libtaglib_plugin_sid.la
libtaglib_plugin_sid_la_LDFLAGS = -fPIC -module
libtaglib_plugin_sid_la_LIBADD = $(TAGLIB_LIBS) $(SID_LIBS)
libtaglib_plugin_sid_la_SOURCES =     \
    plugin.cc                         \
    sidfile.cpp                       \
    sidproperties.cpp                 \
    sidtag.cpp                        \
    resolver.cpp

noinst_HEADERS =                      \
    sidfile.h                         \
    sidproperties.h                   \
    sidtag.h                          \
    resolver.h

libtaglib_plugin_siddir = $(PLUGIN_DIR)/taglib
