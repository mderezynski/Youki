libexec_PROGRAMS = youki-mlibman 

youki_mlibman_LDFLAGS = -Wl,--export-dynamic 

youki_mlibman_CXXFLAGS =                        \
        $(BOOST_CPPFLAGS)                       \
        $(CAIROMM_CFLAGS)                       \
        $(DBUS_CFLAGS)                          \
        $(GIOMM_CFLAGS)                         \
        $(GLADEMM_CFLAGS)                       \
        $(GLIBMM_CFLAGS)                        \
        $(GST_CFLAGS)                           \
        $(GTKMM_CFLAGS)                         \
        $(HAL_CFLAGS)                           \
        $(NM_CFLAGS)                            \
        $(LIBOFA_CFLAGS)                        \
        $(SIGX_CFLAGS)                          \
        $(SOUP_CFLAGS)                          \
        $(SQLITE_CFLAGS)                        \
        $(TAGLIB_CFLAGS)                        \
        $(X_CFLAGS)                             \
        $(XML_CFLAGS)                           \
        $(XPROTO_CFLAGS)                        \
        -I$(top_srcdir)                         \
        -I$(top_srcdir)/include                 \
        -I$(top_srcdir)/libhal++    

youki_mlibman_LDADD =                           \
        $(BOOST_LDFLAGS)                        \
        $(BOOST_REGEX_LIB)                      \
        $(BOOST_FILESYSTEM_LIB)                 \
        $(CAIROMM_LIBS)                         \
        $(DBUS_LIBS)                            \
        $(GIOMM_LIBS)                           \
        $(GLADEMM_LIBS)                         \
        $(GLIBMM_LIBS)                          \
        $(GST_LIBS)                             \
        $(GTKMM_LIBS)                           \
        $(HAL_LIBS)                             \
        $(NM_LIBS)                              \
        $(LIBOFA_LIBS)                          \
        $(SIGX_LIBS)                            \
        $(SM_LIBS)                              \
        $(SOUP_LIBS)                            \
        $(SQLITE_LIBS)                          \
        $(TAGLIB_LIBS)                          \
        $(XML_LIBS)                             \
        $(XLIB_LIBS)                            \
        -lgstinterfaces-0.10                    \
        $(top_builddir)/mcs/libmconfig.la                   \
        $(top_builddir)/api/libmpx-api.la                   \
        $(top_builddir)/widgets/libmpx-widgets.la           \
        $(top_builddir)/musicbrainz/libmpx-musicbrainz.la   


youki_mlibman_CFLAGS = \
    $(GLIB_CFLAGS) $(GST_CFLAGS) -I$(top_srcdir)/include

if HAVE_HAL
youki_mlibman_LDADD += $(top_builddir)/libhal++/libhalcc.la
youki_mlibman_CXXFLAGS += -I$(top_srcdir)/libhal++
endif # HAVE_HAL

youki_mlibman_CXXFLAGS += -D__USE_POSIX199309
youki_mlibman_CFLAGS += -D__USE_POSIX199309

if HAVE_LINUX 
youki_mlibman_CXXFLAGS += -D_XOPEN_SOURCE=600
youki_mlibman_CFLAGS += -D_XOPEN_SOURCE=600
endif

youki_mlibman_SOURCES =                     \
                                            \
        audio-types.hh                      \
        import-folder.hh                    \
        import-share.hh                     \
        library-mlibman.hh                  \
        library-scanner-thread-mlibman.hh   \
        messages.hh                         \
        mlibmanager.hh                      \
        paths.hh                            \
        request-value.hh                    \
        signals.hh                          \
                                            \
        audio.cc                            \
        error.cc                            \
        hal.cc                              \
        import-folder.cc                    \
        import-share.cc                     \
        library-mlibman.cc                  \
        library-scanner-thread-mlibman.cc   \
        main.cc                             \
        metadatareader-taglib.cc            \
        minisoup.cc                         \
        mlibmanager.cc                      \
        network.cc                          \
        paths.cc                            \
        request-value.cc                    \
        services.cc                         \
        signals.cc                          \
        sql.cc                              \
        uri.cc                              \
        util-file.cc                        \
        util-string.cc                      \
        util-ui.cc



EXTRA_DIST =                                \
    dbus-obj-MLIBMAN.xml      


BUILT_SOURCES =                             \
    mpx-mlibman-dbus.hh    

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)

mpx-mlibman-dbus.hh:
	$(DBUSBINDINGTOOL) dbus-obj-MLIBMAN.xml --adaptor=mpx-mlibman-dbus.hh

