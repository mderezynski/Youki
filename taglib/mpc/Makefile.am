INCLUDES = $(TAGLIB_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir) \
							-I$(top_srcdir)/taglib

libtaglib_plugin_mpc_LTLIBRARIES = libtaglib_plugin_mpc.la
libtaglib_plugin_mpc_la_LDFLAGS = -fPIC -module
libtaglib_plugin_mpc_la_LIBADD = $(TAGLIB_LIBS) $(GLIBMM_LIBS)		\
	    $(top_builddir)/taglib/common/libmpx_common_reader.la

libtaglib_plugin_mpc_la_SOURCES =     \
      plugin.cc

libtaglib_plugin_mpcdir = $(PLUGIN_DIR)/taglib
