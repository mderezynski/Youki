lib_LTLIBRARIES = libmpx-api.la

libmpx_api_la_CXXFLAGS = 							\
		$(ALSA_CFLAGS)								\
		$(BOOST_CPPFLAGS)							\
		$(CAIROMM_CFLAGS)   						\
		$(CLUTTER_CFLAGS)							\
		$(CDIO_CFLAGS)								\
		$(CDPARANOIA_INCLUDES) 						\
		$(DBUS_CFLAGS)								\
		$(EXO_CFLAGS)								\
		$(GDL_CFLAGS)								\
		$(GIOMM_CFLAGS)								\
		$(GLADEMM_CFLAGS)							\
		$(GLIBMM_CFLAGS)							\
		$(GST_CFLAGS)								\
		$(GTKGLEXT_CFLAGS)							\
		$(GTKMM_CFLAGS)								\
		$(HAL_CFLAGS)								\
		$(NM_CFLAGS)								\
		$(LIBOFA_CFLAGS)							\
		$(PYGTK_CFLAGS)								\
		$(PYTHON_CFLAGS)							\
		$(RSVG_CFLAGS)								\
		$(SEXYMM_CFLAGS)							\
		$(SIGX_CFLAGS)								\
		$(SOUP_CFLAGS)								\
		$(SQLITE_CFLAGS)							\
		$(STARTUP_NOTIFY_CFLAGS)					\
		$(TAGLIB_CFLAGS)							\
		$(X_CFLAGS)									\
		$(XML_CFLAGS)        						\
		$(XPROTO_CFLAGS)							\
		-I$(top_srcdir)								\
		-I$(top_srcdir)/include	

libmpx_api_la_LIBADD =								\
		$(ALSA_LIBS)								\
		$(BOOST_LDFLAGS)							\
		$(BOOST_REGEX_LIB)							\
		$(BOOST_FILESYSTEM_LIB)						\
		$(BOOST_IOSTREAMS_LIB)						\
		$(BOOST_PYTHON_LIB)							\
		$(CAIROMM_LIBS)								\
		$(CLUTTER_LIBS)								\
		$(CDIO_LIBS)								\
		$(CDPARANOIA_LIBS)							\
		$(DBUS_LIBS)								\
		$(EXO_LIBS)									\
		$(GDL_LIBS)									\
		$(GIOMM_LIBS)								\
		$(GLADEMM_LIBS)								\
		$(GLIBMM_LIBS)								\
		$(GST_LIBS)									\
		$(GTKGLEXT_LIBS)							\
		$(GTKMM_LIBS)								\
		$(HAL_LIBS)									\
		$(NM_LIBS)									\
		$(LIBOFA_LIBS)								\
		$(PYGTK_LIBS)								\
		$(PYTHON_LIBS)								\
		$(RSVG_LIBS)								\
		$(SEXYMM_LIBS)								\
		$(SIGX_LIBS)								\
		$(SM_LIBS)									\
		$(SOUP_LIBS)								\
		$(SQLITE_LIBS)								\
		$(STARTUP_NOTIFY_LIBS)						\
		$(TAGLIB_LIBS)								\
		$(XML_LIBS)									\
		$(XLIB_LIBS)								\
		-lgstinterfaces-0.10 						\
		-lgstvideo-0.10								\
		$(top_builddir)/xmlcpp/libmpx-parsers.la

if HAVE_HAL
libmpx_api_la_LIBADD += $(top_builddir)/libhal++/libhalcc.la
libmpx_api_la_CXXFLAGS += -I$(top_srcdir)/libhal++
endif # HAVE_HAL

libmpx_api_la_CFLAGS =								\
    $(GLIB_CFLAGS) $(GST_CFLAGS) -I$(top_srcdir)/include

libmpx_api_la_SOURCES = 							\
													\
		aque.cc										\
													\
		base64.c									\
		md5.c										\
		random.cc									\
													\
		gstpuid.cc									\
                                                    \
		libxml2-sax-base.cc							\
		xspf-libxml2-sax.cc							\
													\
		lyrics.cc									\
													\
		playlistparser-xspf.cc						\
		playlistparser-pls.cc						\
													\
		xml.cc										\
		xpath.cc									


if HAVE_HAL
libmpx_api_la_SOURCES +=                            \
        hal.cc                                      
												
endif
