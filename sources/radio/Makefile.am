INCLUDES = 						\
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
  	-I$(top_srcdir)

libmpxsource_radio_LTLIBRARIES = libmpxsource-radio.la
libmpxsource_radio_la_LDFLAGS = -fPIC -module
libmpxsource_radio_la_LIBADD =   \
		$(BOOST_LDFLAGS)			\
		$(BOOST_REGEX_LIB)			\
		$(BOOST_FILESYSTEM_LIB)		\
		$(BOOST_IOSTREAMS_LIB)		\
		$(EXO_LIBS)					\
		$(GIOMM_LIBS)				\
		$(GLADEMM_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GTKMM_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(PYTHON_LIBS)				\
		$(PYGTK_LIBS)				\
		$(SOUP_LIBS)				\
		$(top_builddir)/widgets/libmpx-widgets.la


libmpxsource_radio_la_SOURCES =  		\
	radio-directory-types.hh      		\
	radio-directory-view-base.hh  		\
	radio-directory-view-base.cc  		\
	streams-icecast.hh					\
	streams-icecast.cc					\
	streams-shoutcast.hh				\
	streams-shoutcast.cc				\
	source-radio.hh						\
	source-radio.cc						\
	plugin.cc

libmpxsource_radiodir = $(PLUGIN_DIR)/sources

glade_DATA = 						\
	source-radio.glade				
gladedir = $(pkgdatadir)/glade
EXTRA_DIST = $(glade_DATA)
