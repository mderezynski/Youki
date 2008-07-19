glade_DATA = 								\
	source-lastfm.glade						

EXTRA_DIST = $(glade_DATA)

gladedir = $(pkgdatadir)/glade


INCLUDES =							\
		$(BOOST_CPPFLAGS)			\
		$(DBUS_CFLAGS)				\
		$(EXO_CFLAGS)				\
		$(GDL_CFLAGS)				\
		$(GLADEMM_CFLAGS)			\
		$(GLIBMM_CFLAGS)			\
		$(GTKMM_CFLAGS)				\
		$(PYTHON_CFLAGS)			\
		$(PYGTK_CFLAGS)				\
		$(SEXYMM_CFLAGS)			\
		$(SIGX_CFLAGS)				\
		$(SOUP_CFLAGS)				\
		-I$(top_srcdir)/include		\
		-I$(top_srcdir)

libmpxsource_lastfm_LTLIBRARIES = libmpxsource-lastfm.la
libmpxsource_lastfm_la_LDFLAGS = -fPIC -module
libmpxsource_lastfm_la_LIBADD =		\
		$(BOOST_LDFLAGS)			\
		$(BOOST_REGEX_LIB)			\
		$(BOOST_FILESYSTEM_LIB)		\
		$(BOOST_IOSTREAMS_LIB)		\
		$(EXO_LIBS)					\
		$(GDL_LIBS)					\
		$(GLADEMM_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GTKMM_LIBS)				\
		$(PYTHON_LIBS)				\
		$(PYGTK_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(SOUP_LIBS)				


libmpxsource_lastfm_la_SOURCES =			\
		plugin.cc							\
		lastfm.hh							\
		lastfm.cc							\
		source-lastfm.hh					\
		source-lastfm.cc					

libmpxsource_lastfmdir = $(PLUGIN_DIR)/sources
