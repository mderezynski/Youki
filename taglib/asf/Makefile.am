INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_srcdir)/taglib
libtaglib_plugin_asf_LTLIBRARIES = libtaglib_plugin_asf.la
libtaglib_plugin_asf_la_LDFLAGS = -fPIC -module
libtaglib_plugin_asf_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)			\
    $(top_builddir)/taglib/common/libmpx_common_reader.la

libtaglib_plugin_asf_la_SOURCES =     \
    plugin.cc                         \
		resolver.cpp											\
		asfattribute.cpp  								\
		asffile.cpp  											\
		asfproperties.cpp  								\
		asftag.cpp

noinst_HEADERS =                      \
    resolver.h                        \
    asfattribute.h                    \
    asffile.h                         \
    asfproperties.h                   \
    asftag.h

libtaglib_plugin_asfdir = $(PLUGIN_DIR)/taglib
