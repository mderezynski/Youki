libexec_PROGRAMS = youki-bin 

youki_bin_LDFLAGS = -Wl,--export-dynamic 

youki_bin_CXXFLAGS =                            \
        $(ALSA_CFLAGS)                          \
        $(BOOST_CPPFLAGS)                       \
        $(CAIROMM_CFLAGS)                       \
        $(CLUTTER_CFLAGS)                       \
        $(CDIO_CFLAGS)                          \
        $(CDPARANOIA_INCLUDES)                  \
        $(DBUS_CFLAGS)                          \
        $(EXO_CFLAGS)                           \
        $(GDL_CFLAGS)                           \
        $(GIOMM_CFLAGS)                         \
        $(GLADEMM_CFLAGS)                       \
        $(GLIBMM_CFLAGS)                        \
        $(GST_CFLAGS)                           \
        $(GTKGLEXT_CFLAGS)                      \
        $(GTKMM_CFLAGS)                         \
        $(HAL_CFLAGS)                           \
        $(NM_CFLAGS)                            \
        $(LIBOFA_CFLAGS)                        \
        $(PYGTK_CFLAGS)                         \
        $(PYTHON_CFLAGS)                        \
        $(RSVG_CFLAGS)                          \
        $(SEXYMM_CFLAGS)                        \
        $(SIGX_CFLAGS)                          \
        $(SOUP_CFLAGS)                          \
        $(SQLITE_CFLAGS)                        \
        $(STARTUP_NOTIFY_CFLAGS)                \
        $(TAGLIB_CFLAGS)                        \
        $(X_CFLAGS)                             \
        $(XML_CFLAGS)                           \
        $(XPROTO_CFLAGS)                        \
        -I$(top_srcdir)                         \
        -I$(top_srcdir)/include                 \
        -I$(top_srcdir)/libhal++    

youki_bin_LDADD =                               \
        $(ALSA_LIBS)                            \
        $(BOOST_LDFLAGS)                        \
        $(BOOST_REGEX_LIB)                      \
        $(BOOST_FILESYSTEM_LIB)                 \
        $(BOOST_IOSTREAMS_LIB)                  \
        $(BOOST_PYTHON_LIB)                     \
        $(CAIROMM_LIBS)                         \
        $(CLUTTER_LIBS)                         \
        $(CDIO_LIBS)                            \
        $(CDPARANOIA_LIBS)                      \
        $(DBUS_LIBS)                            \
        $(EXO_LIBS)                             \
        $(GDL_LIBS)                             \
        $(GIOMM_LIBS)                           \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GST_LIBS)                             \
        $(GTKGLEXT_LIBS)                        \
        $(GTKMM_LIBS)                           \
        $(HAL_LIBS)                             \
        $(NM_LIBS)                              \
        $(LIBOFA_LIBS)                          \
        $(PYGTK_LIBS)                           \
        $(PYTHON_LIBS)                          \
        $(RSVG_LIBS)                            \
        $(SEXYMM_LIBS)                          \
        $(SIGX_LIBS)                            \
        $(SM_LIBS)                              \
        $(SOUP_LIBS)                            \
        $(SQLITE_LIBS)                          \
        $(STARTUP_NOTIFY_LIBS)                  \
        $(TAGLIB_LIBS)                          \
        $(XML_LIBS)                             \
        $(XLIB_LIBS)                            \
        -lgstinterfaces-0.10                    \
        -lgstvideo-0.10                         \
                                                \
                                                \
        $(top_builddir)/jnetlib/libjnetlib.la               \
        $(top_builddir)/mcs/libmconfig.la                   \
        $(top_builddir)/api/libmpx-api.la                   \
        $(top_builddir)/widgets/libmpx-widgets.la           \
        $(top_builddir)/musicbrainz/libmpx-musicbrainz.la   \
        $(top_builddir)/com/libmpx-view-components.la

youki_bin_CFLAGS =                                          \
    $(GLIB_CFLAGS) $(GST_CFLAGS) -I$(top_srcdir)/include

if HAVE_HAL
youki_bin_LDADD += $(top_builddir)/libhal++/libhalcc.la
youki_bin_CXXFLAGS += -I$(top_srcdir)/libhal++
endif # HAVE_HAL

youki_bin_CXXFLAGS += -D__USE_POSIX199309
youki_bin_CFLAGS += -D__USE_POSIX199309

if HAVE_LINUX 
youki_bin_CXXFLAGS += -D_XOPEN_SOURCE=600
youki_bin_CFLAGS += -D_XOPEN_SOURCE=600
endif

youki_bin_SOURCES =             \
                                \
    artist-images.cc            \
                                \
    audio.cc                    \
    audio-types.hh              \
    audio-exception.hh          \
                                \
    covers.cc                   \
    covers-stores.cc            \
                                \
    dialog-about.hh             \
    dialog-about.cc             \
                                \
    dialog-equalizer.hh         \
    dialog-equalizer.cc         \
                                \
    dialog-filebrowser.hh       \
    dialog-filebrowser.cc       \
                                \
    error.cc                    \
                                \
    flow_engine.hh              \
    flow_engine.cc              \
    flow_widget.hh              \
    flow_widget.cc              \
                                \
    glib-marshalers.c           \
                                \
    import-share.hh             \
    import-share.cc             \
                                \
    import-folder.hh            \
    import-folder.cc            \
                                \
    infoarea.hh                 \
    infoarea.cc                 \
    infoarea__protected.cc      \
    infoarea__private.cc        \
                                \
    jnethttpsrc.h               \
    jnethttpsrc.cc              \
                                \
    kobo-position.hh            \
    kobo-position.cc            \
                                \
    youki-controller.hh         \
    youki-controller.cc         \
                                \
    kobo-cover.hh               \
    kobo-cover.cc               \
                                \
    kobo-main.hh                \
    kobo-main.cc                \
                                \
    kobo-titleinfo.hh           \
    kobo-titleinfo.cc           \
                                \
    kobo-volume.hh              \
    kobo-volume.cc              \
                                \
    library.cc                  \
    library-scanner-thread.cc   \
                                \
    main.cc                     \
    messages.hh                 \
                                \
    markov-analyzer-thread.cc   \
    metadatareader-taglib.cc    \
    minisoup.cc                 \
                                \
    network.cc                  \
                                \
    paths.hh                    \
    paths.cc                    \
                                \
    play.cc                     \
                                \
    playbacksource.cc           \
                                \
    preferences.cc              \
                                \
    request-value.hh            \
    request-value.cc            \
                                \
    sidebar.hh                  \
    sidebar.cc                  \
                                \
    splash-screen.hh            \
    splash-screen.cc            \
                                \
    services.cc                 \
                                \
    signals.cc                  \
    signals.hh                  \
                                \
    sql.cc                      \
                                \
    stock.cc                    \
                                \
    uri.cc                      \
                                \
    util-file.cc                \
    util-graphics.cc            \
    util-string.cc              \
    util-ui.cc                  \
                                \
    video-playback.hh           \
    video-playback.cc           \
                                \
    video-widget.hh             \
    video-widget.cc             \
                                \
    volume-control.hh           \
    volume-control.cc

if HAVE_HAL                             
youki_bin_SOURCES +=            \
    hal.cc                      
endif

BUILT_SOURCES =                 \
    mpx-mlibman-dbus-proxy.hh   \
    mpx-app-dbus-adaptor.hh

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)

mpx-mlibman-dbus-proxy.hh: $(top_srcdir)/dbus/dbus-obj-MLIBMAN.xml
	$(DBUSBINDINGTOOL) $(top_srcdir)/dbus/dbus-obj-MLIBMAN.xml --proxy=mpx-mlibman-dbus-proxy.hh

mpx-app-dbus-adaptor.hh:
	$(DBUSBINDINGTOOL) $(top_srcdir)/dbus/dbus-obj-YOUKI.xml --adaptor=mpx-app-dbus-adaptor.hh
