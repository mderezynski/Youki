libexec_PROGRAMS = youki-mlibman-bin

youki_mlibman_bin_LDFLAGS = -Wl,--export-dynamic 

youki_mlibman_bin_CXXFLAGS =                        \
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

youki_mlibman_bin_LDADD =                       \
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
        $(top_builddir)/mcs/libmconfig.la                   \
        $(top_builddir)/api/libmpx-api.la                   \
        $(top_builddir)/widgets/libmpx-widgets.la           \
        $(top_builddir)/musicbrainz/libmpx-musicbrainz.la   


youki_mlibman_bin_CFLAGS = \
    $(GLIB_CFLAGS) $(GST_CFLAGS) -I$(top_srcdir)/include

if HAVE_HAL
youki_mlibman_bin_LDADD += $(top_builddir)/libhal++/libhalcc.la
youki_mlibman_bin_CXXFLAGS += -I$(top_srcdir)/libhal++
endif # HAVE_HAL

youki_mlibman_bin_CXXFLAGS += -D__USE_POSIX199309
youki_mlibman_bin_CFLAGS += -D__USE_POSIX199309

if HAVE_LINUX 
youki_mlibman_bin_CXXFLAGS += -D_XOPEN_SOURCE=600
youki_mlibman_bin_CFLAGS += -D_XOPEN_SOURCE=600
endif

youki_mlibman_bin_SOURCES =                 \
                                            \
        main.cc                             \
                                            \
        import-folder.hh                    \
        import-share.hh                     \
        library-mlibman.hh                  \
        library-scanner-thread-mlibman.hh   \
        mlibmanager.hh                      \
        request-value.hh                    \
                                            \
        import-folder.cc                    \
        import-share.cc                     \
        library-mlibman.cc                  \
        library-scanner-thread-mlibman.cc   \
        mlibmanager.cc                      \
        request-value.cc                    \
                                            \
        mpx-mlibman-dbus.hh                 


BUILT_SOURCES =                             \
        mpx-mlibman-dbus.hh                 

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)

mpx-mlibman-dbus.hh: $(top_srcdir)/dbus/dbus-obj-MLIBMAN.xml
	$(DBUSBINDINGTOOL) $(top_srcdir)/dbus/dbus-obj-MLIBMAN.xml --adaptor=$(abs_top_srcdir)/mpxlib/mpx-mlibman-dbus.hh

