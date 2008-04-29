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
	podcast-libxml2-sax.cc			\
	podcast-libxml2-sax.hh			\
	podcast-utils.cc				\
	podcast-utils.hh				\
	podcast-v2-types.cc				\
	podcast-v2-types.hh				\
	podcast-cache-libxml2-sax.cc	\
	podcast-cache-libxml2-sax.hh	\
	podcast.cc						\
	podcast.hh

libmpxsource_podcastsdir = $(PLUGIN_DIR)/sources

EXTRA_DIST = glib-marshalers.list

built_header_make = 	\
		glib-marshalers.h

glib-marshalers.c: glib-marshalers.list 
	@echo "* Generating glib-marshalers.c"
	@$(GLIB_GENMARSHAL) --prefix=psm_glib_marshal --body   $(srcdir)/glib-marshalers.list > glib-marshalers.c

glib-marshalers.h: glib-marshalers.list
	@echo "* Generating glib-marshalers.h"
	@$(GLIB_GENMARSHAL) --prefix=psm_glib_marshal --header $(srcdir)/glib-marshalers.list > glib-marshalers.h

