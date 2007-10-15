INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_srcdir)/taglib
libtaglib_plugin_mp4_LTLIBRARIES = libtaglib_plugin_mp4.la
libtaglib_plugin_mp4_la_LDFLAGS = -fPIC -module
libtaglib_plugin_mp4_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)			\
    $(top_builddir)/taglib/common/libmpx_common_reader.la

libtaglib_plugin_mp4_la_SOURCES = \
    plugin.cc          	\
	resolver.cpp		\
	mp4atom.cpp			\
	mp4file.cpp			\
	mp4item.cpp			\
	mp4properties.cpp	\
	mp4tag.cpp

noinst_HEADERS = \
	aux.hh			\
    resolver.h     	\
	mp4atom.h		\
	mp4file.h		\
	mp4item.h		\
	mp4properties.h	\
	mp4tag.h

libtaglib_plugin_mp4dir = $(PLUGIN_DIR)/taglib
