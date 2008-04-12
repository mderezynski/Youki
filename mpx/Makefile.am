SUBDIRS = parser musicbrainz

libexec_PROGRAMS = mpx-bin 

mpx_bin_LDFLAGS = -Wl,--export-dynamic 

mpx_bin_CXXFLAGS = 									\
		$(ALSA_CFLAGS)								\
		$(BOOST_CPPFLAGS)							\
		$(CAIROMM_CFLAGS)   						\
		$(CLUTTER_CFLAGS)							\
		$(CDIO_CFLAGS)								\
		$(CDPARANOIA_INCLUDES) 						\
		$(DBUS_CFLAGS)								\
		$(EXO_CFLAGS)								\
		$(GIOMM_CFLAGS)								\
		$(GLADEMM_CFLAGS)							\
		$(GLIBMM_CFLAGS)							\
		$(GST_CFLAGS)								\
		$(GTKMM_CFLAGS)								\
		$(HAL_CFLAGS)								\
		$(NM_CFLAGS)								\
		$(PYGTK_CFLAGS)								\
		$(PYTHON_CFLAGS)							\
		$(RSVG_CFLAGS)								\
		$(SEXYMM_CFLAGS)							\
		$(SOUP_CFLAGS)								\
		$(SQLITE_CFLAGS)							\
		$(STARTUP_NOTIFY_CFLAGS)					\
		$(TAGLIB_CFLAGS)							\
		$(X_CFLAGS)									\
		$(XML_CFLAGS)        						\
		$(XPROTO_CFLAGS)							\
		-I$(top_srcdir)								\
		-I$(top_srcdir)/include	

mpx_bin_LDADD =										\
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
		$(GIOMM_LIBS)								\
		$(GLADEMM_LIBS)								\
		$(GLIBMM_LIBS)								\
		$(GST_LIBS)									\
		$(GTKMM_LIBS)								\
		$(HAL_LIBS)									\
		$(NM_LIBS)									\
		$(PYGTK_LIBS)								\
		$(PYTHON_LIBS)								\
		$(RSVG_LIBS)								\
		$(SEXYMM_LIBS)								\
		$(SM_LIBS)									\
		$(SOUP_LIBS)								\
		$(SQLITE_LIBS)								\
		$(STARTUP_NOTIFY_LIBS)						\
		$(TAGLIB_LIBS)								\
		$(XML_LIBS)									\
		$(XLIB_LIBS)								\
		-lgstinterfaces-0.10 						\
		-lgstvideo-0.10								\
		$(top_builddir)/jnetlib/libjnetlib.la		\
		$(top_builddir)/widgets/libmpx-widgets.la	\
		$(top_builddir)/xmlcpp/libmpx-parsers.la	\
		$(top_builddir)/mcs/libmconfig.la			\
		$(top_builddir)/mpx/parser/libmpxparser.la

mpx_bin_CFLAGS =				\
    $(GLIB_CFLAGS) $(GST_CFLAGS) -I$(top_srcdir)/include

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
	dialog-filebrowser.hh		\
	dialog-filebrowser.cc		\
								\
	exception.hh				\
	exception-db.hh				\
								\
	main.cc						\
								\
	covers.cc					\
								\
	audio.hh					\
	audio.cc					\
								\
	base64.h					\
	base64.c					\
								\
	mpx-py.hh					\
	mpx-py.cc					\
								\
	import-share.hh				\
	import-share.cc				\
								\
	import-folder.hh			\
	import-folder.cc			\
								\
	jnethttpsrc.h				\
	jnethttpsrc.cc				\
								\
	lastfm.hh					\
								\
	last-fm-xmlrpc.hh			\
	last-fm-xmlrpc.cc			\
								\
	library.cc					\
								\
	lyrics-v2.hh				\
	lyrics-v2.cc				\
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
	playlistparser-xspf.hh		\
	playlistparser-xspf.cc		\
								\
	plugin.hh					\
	plugin.cc					\
	plugin-manager-gui.hh		\
	plugin-manager-gui.cc		\
								\
	preferences.hh				\
	preferences.cc				\
								\
	py-library.hh				\
	py-library.cc				\
								\
	request-value.hh			\
	request-value.cc			\
								\
	sql.hh						\
	sql.cc						\
								\
	stock.hh					\
	stock.cc					\
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
	xml.cc						\
								\
	xspf.hh						\
	xspf-libxml2-sax.hh			\
	xspf-libxml2-sax.cc

if HAVE_HAL								
mpx_bin_SOURCES += hal.hh hal.cc
endif

built_header_make = \
	dbus-obj-MPX-glue.h

EXTRA_DIST = \
	dbus-obj-MPX.xml \
	dbus-marshalers.list

dbus-obj-MPX-glue.h: dbus-obj-MPX.xml
	@echo "* Generating /MPX DBus Glue"
	@dbus-binding-tool --mode=glib-server --prefix=Player::DBusMPX::mpx \
    $(abs_top_builddir)/mpx/dbus-obj-MPX.xml > \
    dbus-obj-MPX-glue.h.temp
	@cat dbus-obj-MPX-glue.h.temp   | sed s:G_BEGIN_DECLS:: | sed s:G_END_DECLS:: > \
    dbus-obj-MPX-glue.h.temp.1
	@cat dbus-obj-MPX-glue.h.temp.1 | sed s/dbus_glib_Player\:\:DBusMPX\:\:mpx/dbus_glib_mpx/g > \
    dbus-obj-MPX-glue.h.temp.2
	@cat dbus-obj-MPX-glue.h.temp.2 | sed s/dbus_glib_marshal_Player\:\:DBusMPX\:\:mpx/dbus_glib_marshal_mpx/g > \
    dbus-obj-MPX-glue.h
	@rm -f dbus-obj-MPX-glue.h.temp
	@rm -f dbus-obj-MPX-glue.h.temp.1
	@rm -f dbus-obj-MPX-glue.h.temp.2

dbus-marshalers.c: dbus-marshalers.list 
	@echo "* Generating dbus-marshalers.c"
	@glib-genmarshal --prefix=mpx_dbus_marshal --body   $(srcdir)/dbus-marshalers.list > dbus-marshalers.c

dbus-marshalers.h: dbus-marshalers.list
	@echo "* Generating dbus-marshalers.h"
	@glib-genmarshal --prefix=mpx_dbus_marshal --header $(srcdir)/dbus-marshalers.list > dbus-marshalers.h


BUILT_SOURCES = dbus-marshalers.c dbus-marshalers.h
mpx_bin_SOURCES += dbus-marshalers.c
