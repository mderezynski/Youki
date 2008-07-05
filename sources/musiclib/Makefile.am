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
		$(SIGX_CFLAGS)				\
		$(SOUP_CFLAGS)				\
		-I$(top_srcdir)/include		\
		-I$(top_srcdir)/libhal++

libmpxsource_musiclib_LTLIBRARIES = libmpxsource-musiclib.la
libmpxsource_musiclib_la_LDFLAGS = -fPIC -module
libmpxsource_musiclib_la_LIBADD =  	\
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


libmpxsource_musiclib_la_SOURCES = 	\
		glib-marshalers.c			\
		plugin.cc					\
		source-musiclib.hh			\
		source-musiclib.cc			

libmpxsource_musiclibdir = $(PLUGIN_DIR)/sources

glade_DATA = 						\
	source-musiclib.glade				
gladedir = $(pkgdatadir)/glade

EXTRA_DIST = glib-marshalers.list

built_header_make = 	\
		glib-marshalers.h

glib-marshalers.c: glib-marshalers.list 
	@echo "* Generating glib-marshalers.c"
	@$(GLIB_GENMARSHAL) --prefix=psm_glib_marshal --body   $(srcdir)/glib-marshalers.list > glib-marshalers.c

glib-marshalers.h: glib-marshalers.list
	@echo "* Generating glib-marshalers.h"
	@$(GLIB_GENMARSHAL) --prefix=psm_glib_marshal --header $(srcdir)/glib-marshalers.list > glib-marshalers.h
