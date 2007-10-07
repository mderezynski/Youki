INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir) \
							-I$(top_srcdir)/taglib

libtaglib_plugin_flac_LTLIBRARIES = libtaglib_plugin_flac.la
libtaglib_plugin_flac_la_LDFLAGS = -fPIC -module
libtaglib_plugin_flac_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS) 		\
			$(top_builddir)/taglib/id3v2/libmpx_id3v2_reader.la 	\
			$(top_builddir)/taglib/xiph/libmpx_xiph_reader.la			\
			$(top_builddir)/taglib/common/libmpx_common_reader.la

libtaglib_plugin_flac_la_SOURCES =     \
      plugin.cc

libtaglib_plugin_flacdir = $(PLUGIN_DIR)/taglib
