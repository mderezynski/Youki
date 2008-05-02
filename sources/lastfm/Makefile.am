images_DATA = artist-unknown.png

imagesdir = $(pkgdatadir)/images

glade_DATA = 								\
	source-lastfm.glade						\
	lastfm-component-top-albums.glade		\
	lastfm-component-user-top-albums.glade

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
		-I$(top_srcdir)/include	

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
		$(SOUP_LIBS)				\
		$(top_builddir)/widgets/libmpx-widgets.la


libmpxsource_lastfm_la_SOURCES =			\
		component-base.hh					\
		component-base.cc					\
		component-top-albums.hh				\
		component-top-albums.cc				\
		component-user-top-albums.hh		\
		component-user-top-albums.cc		\
		component-slot-artist.hh			\
		component-slot-artist.cc			\
		plugin.cc							\
		lastfm.hh							\
		lastfm.cc							\
		source-lastfm.hh					\
		source-lastfm.cc					\
		top-albums-fetch-thread.hh			\
		top-albums-fetch-thread.cc			\
		user-top-albums-fetch-thread.hh		\
		user-top-albums-fetch-thread.cc		\
		xsd-track-toptags.hxx				\
		xsd-track-toptags.cxx				\
		xsd-top-albums.hxx					\
		xsd-top-albums.cxx					\
		xsd-user-top-albums.hxx				\
		xsd-user-top-albums.cxx

libmpxsource_lastfmdir = $(PLUGIN_DIR)/sources
