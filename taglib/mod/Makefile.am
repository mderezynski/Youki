INCLUDES = $(GLIBMM_CFLAGS) $(TAGLIB_CFLAGS) $(MOD_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/include
libtaglib_plugin_mod_LTLIBRARIES = libtaglib_plugin_mod.la
libtaglib_plugin_mod_la_LDFLAGS = -fPIC -module
libtaglib_plugin_mod_la_LIBADD = $(TAGLIB_LIBS) $(MOD_LIBS) $(GLIBMM_LIBS)
libtaglib_plugin_mod_la_SOURCES =     \
    plugin.cc                         \
    modfile.cpp                       \
    modproperties.cpp                 \
    modtag.cpp                        \
    resolver.cpp

noinst_HEADERS =                      \
    modfile.h                         \
    modproperties.h                   \
    modtag.h                          \
    resolver.h

libtaglib_plugin_moddir = $(PLUGIN_DIR)/taglib
