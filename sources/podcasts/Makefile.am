INCLUDES = 							\
		$(BOOST_CPPFLAGS)			\
		$(DBUS_CFLAGS)				\
		$(EXO_CFLAGS)				\
		$(GLADEMM_CFLAGS)			\
		$(GLIBMM_CFLAGS)			\
		$(GTKMM_CFLAGS)				\
		$(HAL_CFLAGS)				\
		$(PYTHON_CFLAGS)			\
		$(PYGTK_CFLAGS)				\
		$(SEXYMM_CFLAGS)			\
		$(SOUP_CFLAGS)				\
		-I$(top_srcdir)/include		\
		-I$(top_srcdir)/libhal++

libmpxsource_podcasts_LTLIBRARIES = libmpxsource-podcasts.la
libmpxsource_podcasts_la_LDFLAGS = -fPIC -module
libmpxsource_podcasts_la_LIBADD =  	\
		$(BOOST_LDFLAGS)			\
		$(BOOST_REGEX_LIB)			\
		$(BOOST_FILESYSTEM_LIB)		\
		$(BOOST_IOSTREAMS_LIB)		\
		$(EXO_LIBS)					\
		$(GLADEMM_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GTKMM_LIBS)				\
		$(HAL_LIBS)					\
		$(PYTHON_LIBS)				\
		$(PYGTK_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(SOUP_LIBS)				\
		$(top_builddir)/widgets/libmpx-widgets.la


libmpxsource_podcasts_la_SOURCES = 	\
		plugin.cc					\
		rss.hxx						\
		rss.cxx						\
		podcast-types.hh			\
		source-podcasts.hh			\
		source-podcasts.cc

libmpxsource_podcastsdir = $(PLUGIN_DIR)/sources

stock_DATA = stock-feed-add.png
stockdir = $(pkgdatadir)/icons/hicolor/24x24/stock 

glade_DATA = 						\
	source-podcasts.glade				
gladedir = $(pkgdatadir)/glade
