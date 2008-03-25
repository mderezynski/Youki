SUBDIRS = parser

libexec_PROGRAMS = mpx-bin 

mpx_bin_LDFLAGS = -Wl,--export-dynamic 

mpx_bin_CXXFLAGS = 				\
    $(ALSA_CFLAGS)				\
    $(BOOST_CPPFLAGS)			\
	$(CAIROMM_CFLAGS)   		\
	$(CLUTTER_CFLAGS)			\
    $(CDIO_CFLAGS)				\
    $(CDPARANOIA_INCLUDES) 		\
	$(DBUS_CFLAGS)				\
	$(EXO_CFLAGS)				\
	$(GIO_CFLAGS)				\
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

mpx_bin_LDADD =						\
		$(ALSA_LIBS)				\
		$(BOOST_LDFLAGS)			\
		$(BOOST_REGEX_LIB)			\
		$(BOOST_FILESYSTEM_LIB)		\
		$(BOOST_IOSTREAMS_LIB)		\
		$(CAIROMM_LIBS)				\
		$(CLUTTER_LIBS)				\
		$(CDIO_LIBS)				\
		$(CDPARANOIA_LIBS)			\
		$(DBUS_LIBS)				\
		$(EXO_LIBS)					\
		$(GIO_LIBS)					\
		$(GLADEMM_LIBS)				\
		$(GLIBMM_LIBS)				\
		$(GST_LIBS)					\
		$(GTKMM_LIBS)				\
		$(HAL_LIBS)					\
		$(NM_LIBS)					\
		$(RSVG_LIBS)				\
		$(SEXYMM_LIBS)				\
		$(SM_LIBS)					\
		$(SOUP_LIBS)				\
		$(SQLITE_LIBS)				\
		$(STARTUP_NOTIFY_LIBS)		\
		$(TAGLIB_LIBS)				\
		$(XML_LIBS)					\
		$(XLIB_LIBS)				

mpx_bin_LDADD += 				\
		-lgstinterfaces-0.10 	\
		-lgstvideo-0.10

mpx_bin_LDADD +=                                	\
		$(top_builddir)/jnetlib/libjnetlib.la		\
		$(top_builddir)/widgets/libmpx-widgets.la	\
		$(top_builddir)/xmlcpp/libmpx-parsers.la	\
		$(top_builddir)/mcs/libmconfig.la			\
		$(top_builddir)/mpx/parser/libmpxparser.la

mpx_bin_CFLAGS =				\
    $(GLIB_CFLAGS) $(GST_CFLAGS)

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
	widgetloader.h				\
								\
	exception.hh				\
	exception-db.hh				\
								\
	main.cc						\
	types.cc					\
								\
	amazon.cc					\
								\
	audio.hh					\
	audio.cc					\
								\
	base64.h					\
	base64.c					\
								\
	import-share.hh				\
	import-share.cc				\
								\
	import-folder.hh			\
	import-folder.cc			\
								\
	library.cc					\
								\
	metadatareader-taglib.hh	\
	metadatareader-taglib.cc	\
								\
	minisoup.hh					\
	minisoup.cc					\
								\
	md5.h						\
	md5.c						\
								\
	mpx.hh						\
	mpx.cc						\
								\
	mpx-sources.hh				\
	mpx-sources.cc				\
								\
	network.hh					\
	network.cc					\
								\
	play.hh						\
	play.cc						\
								\
	playbacksource.hh			\
	playbacksource.cc			\
								\
	request-value.hh			\
	request-value.cc			\
								\
	source-musiclib.hh			\
	source-musiclib.cc			\
								\
	sql.hh						\
	sql.cc						\
								\
	tasks.hh					\
	tasks.cc					\
								\
	uri.hh						\
	uri.cc						\
								\
	util-file.hh				\
	util-file.cc				\
								\
	util-graphics.hh			\
	util-graphics.cc			\
								\
	util-string.hh				\
	util-string.cc				\
								\
	util-ui.hh					\
	util-ui.cc					\
								\
	video-playback.hh			\
	video-playback.cc			\
								\
	xspf.hh						\
	xspf-libxml2-sax.hh			\
	xspf-libxml2-sax.cc

if HAVE_HAL								
mpx_bin_SOURCES += hal.hh hal.cc
endif
