lib_LTLIBRARIES = libmpx-view-components.la

glade_DATA = 						\
	album-info-window.glade

gladedir = $(pkgdatadir)/glade

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
		-I$(top_srcdir)				

libmpx_view_components_la_LIBADD =  	\
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
		$(SEXYMM_LIBS)				

libmpx_view_components_la_SOURCES =	\
	album-info-window.cc			\
	view-albums.cc					\
	file-system-tree.cc				\
	tagview.cc						\
	treeview-popup.cc				
