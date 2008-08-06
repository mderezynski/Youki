libexec_PROGRAMS = audiosource-bin 

audiosource_bin_LDFLAGS = -Wl,--export-dynamic 

audiosource_bin_CXXFLAGS = 									\
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

audiosource_bin_LDADD =										\
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
													\
													\
		$(top_builddir)/jnetlib/libjnetlib.la						\
		$(top_builddir)/mcs/libmconfig.la							\
		$(top_builddir)/api/libmpx-api.la							\
		$(top_builddir)/widgets/libmpx-widgets.la					\
		$(top_builddir)/musicbrainz/libmpx-musicbrainz.la			\
		$(top_builddir)/viewcomponents/libmpx-view-components.la

audiosource_bin_CFLAGS =				\
    $(GLIB_CFLAGS) $(GST_CFLAGS) -I$(top_srcdir)/include

if HAVE_HAL
audiosource_bin_LDADD += $(top_builddir)/libhal++/libhalcc.la
audiosource_bin_CXXFLAGS += -I$(top_srcdir)/libhal++
endif # HAVE_HAL

audiosource_bin_CXXFLAGS += -D__USE_POSIX199309
audiosource_bin_CFLAGS += -D__USE_POSIX199309

if HAVE_LINUX 
audiosource_bin_CXXFLAGS += -D_XOPEN_SOURCE=600
audiosource_bin_CFLAGS += -D_XOPEN_SOURCE=600
endif

audiosource_bin_SOURCES =				\
								\
	audio.cc					\
	audio-types.hh				\
	audio-exception.hh			\
								\
	covers.cc					\
	covers-stores.cc 			\
								\
	dialog-about.hh				\
	dialog-about.cc				\
								\
	dialog-filebrowser.hh		\
	dialog-filebrowser.cc		\
								\
	error.cc					\
								\
	flow_engine.hh				\
	flow_engine.cc				\
	flow_widget.hh				\
	flow_widget.cc				\
								\
	gtkmmmodule.h				\
	gtkmmmodule.cc				\
	gtkmm.cc					\
								\
	import-share.hh				\
	import-share.cc				\
								\
	import-folder.hh			\
	import-folder.cc			\
								\
	infoarea.hh					\
	infoarea.cc					\
	infoarea__protected.cc		\
	infoarea__private.cc		\
								\
	jnethttpsrc.h				\
	jnethttpsrc.cc				\
								\
	library.cc					\
	library-scanner-thread.cc	\
								\
	main.cc						\
								\
	markov-analyzer-thread.cc	\
								\
	messages.hh					\
								\
	metadatareader-taglib.cc	\
								\
	minisoup.cc					\
								\
	mlibmanager.hh				\
	mlibmanager.cc				\
								\
	mpx.hh						\
	mpx.cc						\
	mpx-mmkeys.cc				\
	mpx-source-handling.cc		\
								\
	network.cc					\
								\
	paths.hh					\
	paths.cc					\
								\
	play.hh						\
	play.cc						\
								\
	playbacksource.cc			\
								\
	plugin.hh					\
	plugin.cc					\
	plugin-manager-gui.hh		\
	plugin-manager-gui.cc		\
								\
	preferences.hh				\
	preferences.cc				\
								\
	pysigc.hh					\
	python.cc					\
								\
	request-value.hh			\
	request-value.cc			\
								\
	sidebar.hh					\
	sidebar.cc					\
								\
	source-controller.hh		\
	source-controller.cc		\
								\
	splash-screen.hh			\
	splash-screen.cc			\
								\
	services.cc					\
								\
	signals.cc					\
	signals.hh					\
								\
	sql.cc						\
								\
	stock.cc					\
								\
	uri.cc						\
								\
	util-file.cc				\
	util-graphics.cc			\
	util-string.cc				\
	util-ui.cc					\
								\
	video-playback.hh			\
	video-playback.cc			\
								\
	video-widget.hh				\
	video-widget.cc				

if HAVE_HAL								
audiosource_bin_SOURCES += hal.cc
endif

BUILT_SOURCES = 			\
	dbus-obj-MPX-glue.h		\
	dbus-obj-PLAYER-glue.h

EXTRA_DIST = 				\
	dbus-obj-MPX.xml 		\
	dbus-obj-PLAYER.xml 	\
	dbus-marshalers.list

dbus-obj-MPX-glue.h: Makefile.am dbus-obj-MPX.xml
	@echo "* Generating /MPX DBus Glue"
	@$(DBUSBINDINGTOOL) --mode=glib-server --prefix=Player::DBusMPX::mpx \
    $(abs_top_srcdir)/mpx/dbus-obj-MPX.xml > \
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

dbus-obj-PLAYER-glue.h: Makefile.am dbus-obj-PLAYER.xml
	@echo "* Generating /Player DBus Glue"
	@$(DBUSBINDINGTOOL) --mode=glib-server --prefix=Player::DBusPlayer::player \
    $(abs_top_srcdir)/mpx/dbus-obj-PLAYER.xml > \
    dbus-obj-PLAYER-glue.h.temp
	@cat dbus-obj-PLAYER-glue.h.temp   | sed s:G_BEGIN_DECLS:: | sed s:G_END_DECLS:: > \
    dbus-obj-PLAYER-glue.h.temp.1
	@cat dbus-obj-PLAYER-glue.h.temp.1 | sed s/dbus_glib_Player\:\:DBusPlayer\:\:player/dbus_glib_player/g > \
    dbus-obj-PLAYER-glue.h.temp.2
	@cat dbus-obj-PLAYER-glue.h.temp.2 | sed s/dbus_glib_marshal_Player\:\:DBusPlayer\:\:player/dbus_glib_marshal_player/g > \
    dbus-obj-PLAYER-glue.h
	@rm -f dbus-obj-PLAYER-glue.h.temp
	@rm -f dbus-obj-PLAYER-glue.h.temp.1
	@rm -f dbus-obj-PLAYER-glue.h.temp.2

dbus-marshalers.c: dbus-marshalers.list 
	@echo "* Generating dbus-marshalers.c"
	@$(GLIB_GENMARSHAL) --prefix=mpx_dbus_marshal --body   $(srcdir)/dbus-marshalers.list > dbus-marshalers.c

dbus-marshalers.h: dbus-marshalers.list
	@echo "* Generating dbus-marshalers.h"
	@$(GLIB_GENMARSHAL) --prefix=mpx_dbus_marshal --header $(srcdir)/dbus-marshalers.list > dbus-marshalers.h


BUILT_SOURCES += dbus-marshalers.c dbus-marshalers.h
audiosource_bin_SOURCES += dbus-marshalers.c

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)
