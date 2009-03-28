libexec_PROGRAMS = youki-mlibman 

youki_mlibman_LDFLAGS = -Wl,--export-dynamic 

youki_mlibman_CXXFLAGS =                        \
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

youki_mlibman_LDADD =                           \
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
        $(top_builddir)/mcs/libmconfig.la                   

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

youki_mlibman_SOURCES =         \
                                \
    audio.cc                    \
    audio-types.hh              \
    audio-exception.hh          \
                                \
    error.cc                    \
                                \
    library.cc                  \
    library-scanner-thread.cc   \
                                \
    main.cc                     \
    messages.hh                 \
                                \
    metadatareader-taglib.cc    \
    minisoup.cc                 \
                                \
    network.cc                  \
                                \
    paths.hh                    \
    paths.cc                    \
                                \
    services.cc                 \
                                \
    signals.cc                  \
    signals.hh                  \
                                \
    sql.cc                      \
                                \
    uri.cc                      \
                                \
    util-file.cc                \
    util-string.cc             
                                
if HAVE_HAL                             
youki_mlibman_SOURCES +=        \
    hal.cc                      \
    mlibmanager.hh              \
    mlibmanager.cc              
endif

EXTRA_DIST =                    \
    dbus-obj-MLIBMAN.xml      

BUILT_SOURCES =                 \
    mpx-mlibman-dbus.hh    

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)

mpx-mlibman-dbus.hh:
	$(DBUSBINDINGTOOL) dbus-obj-MLIBMAN.xml --adaptor=mpx-mlibman-dbus.hh
