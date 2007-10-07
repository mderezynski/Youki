INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include
libtaglib_plugin_audible_LTLIBRARIES = libtaglib_plugin_audible.la
libtaglib_plugin_audible_la_LDFLAGS = -fPIC -module
libtaglib_plugin_audible_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)
libtaglib_plugin_audible_la_SOURCES =     \
    plugin.cc                             \
    audibleproperties.cpp                 \
    audibletag.cpp                        \
    taglib_audiblefile.cpp                \
    resolver.cpp

noinst_HEADERS =                          \
    audibleproperties.h                   \
    audibletag.h                          \
    taglib_audiblefile.h                  \
    resolver.h

libtaglib_plugin_audibledir = $(PLUGIN_DIR)/taglib
