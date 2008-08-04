images_DATA =						\
		play.png					

imagesdir = $(pkgdatadir)/images
EXTRA_DIST = $(images_DATA)

INCLUDES = 							\
		$(BOOST_CPPFLAGS)			\
		$(DBUS_CFLAGS)				\
		$(EXO_CFLAGS)				\
		$(GIOMM_CFLAGS)				\
		$(GLADEMM_CFLAGS)			\
		$(GLIBMM_CFLAGS)			\
		$(GTKMM_CFLAGS)				\
		$(HAL_CFLAGS)				\
		$(PYTHON_CFLAGS)			\
		$(PYGTK_CFLAGS)				\
		$(SEXYMM_CFLAGS)			\
		$(SIGX_CFLAGS)				\
		$(SOUP_CFLAGS)				\
		-I$(top_srcdir)/include		\
		-I$(top_srcdir)				\
		-I$(top_srcdir)/libhal++

libmpxsource_musiclib_LTLIBRARIES = libmpxsource-musiclib.la
libmpxsource_musiclib_la_LDFLAGS = -fPIC -module
libmpxsource_musiclib_la_LIBADD =  	\
		$(BOOST_LDFLAGS)			\
		$(BOOST_REGEX_LIB)			\
		$(BOOST_FILESYSTEM_LIB)		\
		$(BOOST_IOSTREAMS_LIB)		\
		$(EXO_LIBS)					\
		$(GIOMM_LIBS)				\
		$(GLADEMM_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GTKMM_LIBS)				\
		$(HAL_LIBS)					\
		$(PYTHON_LIBS)				\
		$(PYGTK_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(SOUP_LIBS)				\
		$(top_builddir)/widgets/libmpx-widgets.la


libmpxsource_musiclib_la_SOURCES = 		\
		plugin.cc						\
										\
		glib-marshalers.c				\
		listview.hh						\
		musiclib-py.cc					\
		musiclib-py.hh					\
		musiclib-album-info-window.hh	\
		musiclib-album-info-window.cc	\
		musiclib-view-albums.hh			\
		musiclib-view-albums.cc			\
		source-musiclib.hh				\
		source-musiclib.cc				\
		xsd-top-albums.cxx				\
		xsd-top-albums.hxx				\
		xsd-tag-topalbums.hxx			\
		xsd-tag-topalbums.cxx			\
		top-albums-fetch-thread.cc		\
		top-albums-fetch-thread.hh

libmpxsource_musiclibdir = $(PLUGIN_DIR)/sources

glade_DATA = 						\
	source-musiclib.glade			\
	albuminfowindow.glade

gladedir = $(pkgdatadir)/glade

EXTRA_DIST += glib-marshalers.list $(glade_DATA)

BUILT_SOURCES = glib-marshalers.h 
CLEANFILES = $(BUILT_SOURCES) 
DISTCLEANFILES = $(BUILT_SOURCES) 

glib-marshalers.c: glib-marshalers.list 
	@echo "* Generating glib-marshalers.c"
	@$(GLIB_GENMARSHAL) --prefix=psm_glib_marshal --body   $(srcdir)/glib-marshalers.list > glib-marshalers.c

glib-marshalers.h: glib-marshalers.list
	@echo "* Generating glib-marshalers.h"
	@$(GLIB_GENMARSHAL) --prefix=psm_glib_marshal --header $(srcdir)/glib-marshalers.list > glib-marshalers.h

