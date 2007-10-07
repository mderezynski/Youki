INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include
libtaglib_plugin_rmff_LTLIBRARIES = libtaglib_plugin_rmff.la
libtaglib_plugin_rmff_la_LDFLAGS = -fPIC -module
libtaglib_plugin_rmff_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)
libtaglib_plugin_rmff_la_SOURCES =          \
    plugin.cc                               \
    rmff.cpp                                \
    taglib_realmediafile.cpp                \
    resolver.cpp

noinst_HEADERS =                            \
    rmff.h                                  \
    taglib_realmediafile.h                  \
    resolver.h

libtaglib_plugin_rmffdir = $(PLUGIN_DIR)/taglib
