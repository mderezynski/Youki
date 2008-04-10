glade_DATA = 						\
	source-lastfm.glade				

gladedir = $(pkgdatadir)/glade


INCLUDES =							\
		$(BOOST_CPPFLAGS)			\
		$(DBUS_CFLAGS)				\
		$(EXO_CFLAGS)				\
		$(GLADEMM_CFLAGS)			\
		$(GLIBMM_CFLAGS)			\
		$(GTKMM_CFLAGS)				\
		$(PYTHON_CFLAGS)			\
		$(PYGTK_CFLAGS)				\
		$(SEXYMM_CFLAGS)			\
		$(SOUP_CFLAGS)				\
		-I$(top_srcdir)/include	

libmpxsource_lastfm_LTLIBRARIES = libmpxsource-lastfm.la
libmpxsource_lastfm_la_LDFLAGS = -fPIC -module
libmpxsource_lastfm_la_LIBADD =		\
		$(BOOST_LDFLAGS)			\
		$(BOOST_REGEX_LIB)			\
		$(BOOST_FILESYSTEM_LIB)		\
		$(BOOST_IOSTREAMS_LIB)		\
		$(EXO_LIBS)					\
		$(GLADEMM_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GTKMM_LIBS)				\
		$(PYTHON_LIBS)				\
		$(PYGTK_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(SOUP_LIBS)				\
		$(top_builddir)/widgets/libmpx-widgets.la


libmpxsource_lastfm_la_SOURCES =	\
		plugin.cc					\
		lastfm.hh					\
		lastfm.cc					\
		source-lastfm.hh			\
		source-lastfm.cc			\
		xsd-track-toptags.cxx

libmpxsource_lastfmdir = $(PLUGIN_DIR)/sources
