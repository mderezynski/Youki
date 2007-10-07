INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir) -I$(top_srcdir)/taglib
libtaglib_plugin_mp3_LTLIBRARIES = libtaglib_plugin_mp3.la
libtaglib_plugin_mp3_la_LDFLAGS = -fPIC -module
libtaglib_plugin_mp3_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)			\
    $(top_builddir)/taglib/common/libmpx_common_reader.la

libtaglib_plugin_mp3_la_SOURCES =     \
    plugin.cc                         

libtaglib_plugin_mp3dir = $(PLUGIN_DIR)/taglib
