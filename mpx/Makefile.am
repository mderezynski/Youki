libexec_PROGRAMS = mpx-bin 

mpx_bin_LDFLAGS = -Wl,--export-dynamic 

mpx_bin_LDADD =						\
		$(ALSA_LIBS)				\
		$(BOOST_LDFLAGS)			\
		$(CDIO_LIBS)				\
		$(CDPARANOIA_LIBS)			\
		$(DBUS_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GST_LIBS)					\
		$(GTKMM_LIBS)				\
		$(HAL_LIBS)					\
		$(NM_LIBS)					\
		$(RSVG_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(SOUP_LIBS)				\
		$(SQLITE_LIBS)				\
		$(TAGLIB_LIBS)				\
		$(XML_LIBS)					

mpx_bin_LDADD += -lgstinterfaces-0.10 -lgstvideo-0.10

mpx_bin_LDADD +=                                	\
		$(top_builddir)/jnetlib/libjnetlib.la		\
		$(top_builddir)/widgets/libmpxwidgets.la

mpx_bin_CFLAGS =				\
    $(GLIB_CFLAGS) $(GST_CFLAGS)

mpx_bin_CXXFLAGS = 				\
    $(ALSA_CFLAGS)				\
    $(BOOST_CPPFLAGS)			\
	$(CAIROMM_CFLAGS)   		\
    $(CDIO_CFLAGS)				\
    $(CDPARANOIA_INCLUDES) 		\
	$(DBUS_CFLAGS)				\
	$(GLADEMM_CFLAGS)			\
	$(GLIBMM_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GTKMM_CFLAGS)				\
	$(HAL_CFLAGS)				\
	$(NM_CFLAGS)				\
	$(RSVG_CFLAGS)				\
	$(SEXYMM_CFLAGS)			\
	$(SOUP_CFLAGS)				\
	$(SQLITE_CFLAGS)			\
	$(STARTUP_NOTIFY_CFLAGS)	\
	$(TAGLIB_CFLAGS)			\
	$(X_CFLAGS)					\
	$(XML_CFLAGS)        		\
	$(XPROTO_CFLAGS)			\
	-I$(top_srcdir)				\
  	-I$(top_srcdir)/include	

mpx_bin_LDADD +=				\
	$(STARTUP_NOTIFY_LIBS)		\
	$(XLIB_LIBS) $(SM_LIBS)		\
	$(GTKMM_LIBS)				\
	$(GLADEMM_LIBS)				\
	$(CAIROMM_LIBS)				\
	$(BOOST_REGEX_LIB)			\
	$(BOOST_FILESYSTEM_LIB)		\
	$(BOOST_IOSTREAMS_LIB)			

if HAVE_HAL
mpx_bin_LDADD += $(top_builddir)/libhal++/libhalcc.la
mpx_bin_CXXFLAGS += -I$(top_srcdir)/libhal++
endif # HAVE_HAL

mpx_bin_CXXFLAGS += -D__USE_POSIX199309
mpx_bin_CFLAGS += -D__USE_POSIX199309

if HAVE_LINUX 
mpx_bin_CXXFLAGS += -D_XOPEN_SOURCE=600
mpx_bin_CFLAGS += -D_XOPEN_SOURCE=600
endif
  
mpx_bin_SOURCES =				\
								\
	exception.hh				\
	exception-db.hh				\
								\
	main.cc						\
	types.cc					\
								\
	tasks.hh					\
	tasks.cc					\
								\
	audio.hh					\
	audio.cc					\
								\
	hal.hh						\
	hal.cc						\
								\
	sql.hh						\
	sql.cc						\
								\
	library.hh					\
	library.cc					\
								\
	uri.hh						\
	uri.cc						\
								\
	util-file.hh				\
	util-file.cc				\
								\
	util-string.hh				\
	util-string.cc				\
								\
	metadatareader-taglib.hh	\
	metadatareader-taglib.cc
